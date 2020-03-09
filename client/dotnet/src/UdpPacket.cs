using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;

namespace FfClient
{
    internal class UdpPacket
    {
        public byte[] Packet { get; set; }

        public int Length { get; set; }
    }
}
