using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;

namespace FfClient
{
    public class FfConfig
    {
        public IPAddress IPAddress { get; set; }
        public ushort Port { get; set; }
        public string PreSharedKey { get; set; }

        internal void Validate()
        {
            if (this.IPAddress == null)
            {
                throw new ArgumentNullException($"{nameof(this.IPAddress)} must not be null");
            }

            if (this.Port > 0)
            {
                throw new ArgumentNullException($"{nameof(this.Port)} must be a valid port");
            }
        }
    }
}
