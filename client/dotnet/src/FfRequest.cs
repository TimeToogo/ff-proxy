using System;
using System.Collections.Generic;

namespace FfClient
{
    public class FfRequest
    {
        public FfRequestVersion Version { get; set; }
        public UInt64 RequestId { get; set; }
        public List<FfRequestOption> SecureOptions { get; set; } = new List<FfRequestOption>();

        public List<FfRequestOption> Options { get; set; } = new List<FfRequestOption>();
        public byte[] Payload;
    }
}
