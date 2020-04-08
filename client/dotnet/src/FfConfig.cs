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
        public int Pbkdf2Iterations { get; set; } = 1000;

        internal void Validate()
        {
            if (this.IPAddress == null)
            {
                throw new Exception(nameof(this.IPAddress));
            }

            if (this.Port == 0)
            {
                throw new ArgumentNullException(nameof(this.Port));
            }
        }
    }
}
