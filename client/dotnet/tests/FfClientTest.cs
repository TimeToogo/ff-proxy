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
    public class FfClientTest
    {
        private FfConfig MockConfig(string preSharedKey = null)
        {
            return new FfConfig()
            {
                IPAddress = IPAddress.Loopback,
                Port = 8080,
                PreSharedKey = preSharedKey
            };
        }

        [Fact]
        public void TestWriteNumberUInt16()
        {
            var client = new FfClient(this.MockConfig());

            var buff = new byte[2];
            int count = 0;

            client.WriteNumber<ushort>(buff, 0xFF00, ref count);

            Assert.Equal(new byte[] { 255, 0 }, buff);
            Assert.Equal(2, count);
        }

        [Fact]
        public void TestWriteNumberUInt32()
        {
            var client = new FfClient(this.MockConfig());

            var buff = new byte[4];
            int count = 0;

            client.WriteNumber<uint>(buff, 0xFFFF0000, ref count);

            Assert.Equal(new byte[] { 255, 255, 0, 0 }, buff);
            Assert.Equal(4, count);
        }

        [Fact]
        public async Task TestSerializeGetRequest()
        {
            var client = new FfClient(this.MockConfig());

            var request = new HttpRequestMessage(HttpMethod.Get, "http://google.com");

            var serialized = Encoding.UTF8.GetString(await client.SerializeHttpMessage(request));

            Assert.Equal("GET / HTTP/1.1\nHost: google.com\n\n", serialized);
        }

        [Fact]
        public async Task TestSerializePostRequest()
        {
            var client = new FfClient(this.MockConfig());

            var request = new HttpRequestMessage(HttpMethod.Post, "http://google.com")
            {
                Content = new ByteArrayContent(Encoding.UTF8.GetBytes("Body"))
                {
                    Headers = { ContentLength = 4 }
                }
            };

            var serialized = Encoding.UTF8.GetString(await client.SerializeHttpMessage(request));

            Assert.Equal("POST / HTTP/1.1\nHost: google.com\nContent-Length: 4\n\nBody", serialized);
        }

        [Fact]
        public void TestWriteNumberUInt64()
        {
            var client = new FfClient(this.MockConfig());

            var buff = new byte[8];
            int count = 0;

            client.WriteNumber<ulong>(buff, 0xFF00FF00FF00FF00, ref count);

            Assert.Equal(new byte[] { 255, 0, 255, 0, 255, 0, 255, 0 }, buff);
            Assert.Equal(8, count);
        }

        [Fact]
        public async Task TestUnencryptedHttpRequest()
        {
            var client = new FfClient(this.MockConfig());

            var request = new HttpRequestMessage()
            {
                Method = HttpMethod.Get,
                RequestUri = new Uri("http://google.com/"),
            };
            request.Headers.Host = "google.com";

            var packets = await client.CreateRequestPackets(request);

            Assert.Equal(1, (int)packets.Count);

            var packet = packets[0].Packet;
            var requestPayload = "GET / HTTP/1.1\nHost: google.com\n\n";
            var payloadLength = (uint)requestPayload.Length;
            var i = 0;

            Assert.Equal((ushort)FfRequestVersion.V1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Request ID (uint64)
            Assert.NotEqual(new byte[8], packet.Skip(i).Take(8).ToArray());
            i += 8;
            // Request length (uint32)
            Assert.Equal(payloadLength, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet, i)));
            i += 4;
            // Chunk offset (uint32)
            Assert.Equal(0u, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet, i)));
            i += 4;
            // Chunk length (uint16)
            Assert.Equal((ushort)payloadLength, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;

            // EOL option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_EOL, packet[i++]);
            // Option length (uint16)
            Assert.Equal(0, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;

            // Payload
            Assert.Equal(Encoding.UTF8.GetBytes(requestPayload), packet.Skip(i).Take((int)payloadLength).ToArray());
            i += (int)payloadLength;

            Assert.Equal(i, packets[0].Length);
        }


        [Fact]
        public async Task TestEncryptedHttpsRequest()
        {
            var client = new FfClient(this.MockConfig(preSharedKey: "abc"));

            var request = new HttpRequestMessage()
            {
                Method = HttpMethod.Get,
                RequestUri = new Uri("https://google.com/"),
            };

            var packets = await client.CreateRequestPackets(request);

            Assert.Equal(1, (int)packets.Count);

            var packet = packets[0].Packet;
            var requestPayload = "GET / HTTP/1.1\nHost: google.com\n\n";
            var payloadLength = (uint)requestPayload.Length;
            var i = 0;

            Assert.Equal((ushort)FfRequestVersion.V1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Request ID (uint64)
            Assert.NotEqual(new byte[8], packet.Skip(i).Take(8).ToArray());
            i += 8;
            // Request length (uint32)
            Assert.Equal(payloadLength, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet, i)));
            i += 4;
            // Chunk offset (uint32)
            Assert.Equal(0u, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet, i)));
            i += 4;
            // Chunk length (uint16)
            Assert.Equal((ushort)payloadLength, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;

            // HTTPS option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_HTTPS, packet[i++]);
            // Option length (uint16)
            Assert.Equal(1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.Equal(1, packet[i++]);

            // Encryption mode option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_ENCRYPTION_MODE, packet[i++]);
            // Option length (uint16)
            Assert.Equal(1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.Equal((byte)FfEncryptionMode.AES_256_GCM, packet[i++]);

            // Encryption IV option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_ENCRYPTION_IV, packet[i++]);
            // Option length (uint16)
            Assert.Equal(12, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.NotEqual(new byte[12], packet.Skip(i).Take(12).ToArray());
            i += 12;

            // Encryption tag option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_ENCRYPTION_TAG, packet[i++]);
            // Option length (uint16)
            Assert.Equal(16, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.NotEqual(new byte[16], packet.Skip(i).Take(16).ToArray());
            i += 16;

            // Key derive mode option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_KEY_DERIVE_MODE, packet[i++]);
            // Option length (uint16)
            Assert.Equal(1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.Equal((byte)FfKeyDeriveMode.PBKDF2, packet[i++]);

            // Key derive salt option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_KEY_DERIVE_SALT, packet[i++]);
            // Option length (uint16)
            Assert.Equal(16, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;
            // Option value
            Assert.NotEqual(new byte[16], packet.Skip(i).Take(16).ToArray());
            i += 16;

            // EOL option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_EOL, packet[i++]);
            // Option length (uint16)
            Assert.Equal(0, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet, i)));
            i += 2;

            // Payload (encrypted)
            Assert.NotEqual(new byte[payloadLength], packet.Skip(i).Take((int)payloadLength).ToArray());
            Assert.NotEqual(Encoding.UTF8.GetBytes(requestPayload), packet.Skip(i).Take((int)payloadLength).ToArray());
        }


        [Fact]
        public async Task TestMultiPacketHttpRequest()
        {
            var client = new FfClient(this.MockConfig());

            var requestBody = "".PadRight(2000, 'a');
            var request = new HttpRequestMessage()
            {
                Method = HttpMethod.Get,
                RequestUri = new Uri("https://google.com/"),
                Content = new ByteArrayContent(Encoding.UTF8.GetBytes(requestBody))
            };
            request.Headers.Host = "google.com";

            var packets = await client.CreateRequestPackets(request);

            Assert.Equal(2, (int)packets.Count);

            var requestPayload = $"GET / HTTP/1.1\nHost: google.com\nContent-Length: 2000\n\n{requestBody}";
            var payloadLength = (uint)requestPayload.Length;

            // -- Packet 1 --

            var packet1 = packets[0].Packet;
            var i1 = 0;
            int chunkLength1 = 1273;

            Assert.Equal((ushort)FfRequestVersion.V1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet1, i1)));
            i1 += 2;
            // Request ID (uint64)
            Assert.NotEqual(new byte[8], packet1.Skip(i1).Take(8).ToArray());
            i1 += 8;
            // Request length (uint32)
            Assert.Equal(payloadLength, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet1, i1)));
            i1 += 4;
            // Chunk offset (uint32)
            Assert.Equal(0u, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet1, i1)));
            i1 += 4;
            // Chunk length (uint16)
            Assert.Equal(chunkLength1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet1, i1)));
            i1 += 2;

            // HTTPS option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_HTTPS, packet1[i1++]);
            // Option length (uint16)
            Assert.Equal(1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet1, i1)));
            i1 += 2;
            // Option value
            Assert.Equal(1, packet1[i1++]);

            // EOL option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_EOL, packet1[i1++]);
            // Option length (uint16)
            Assert.Equal(0, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet1, i1)));
            i1 += 2;

            // Payload
            Assert.Equal(
                Encoding.UTF8.GetBytes(requestPayload).Take(chunkLength1).ToArray(),
                packet1.Skip(i1).Take(chunkLength1).ToArray()
            );

            // -- Packet 2 --

            var packet2 = packets[1].Packet;
            var i2 = 0;
            int chunkLength2 = (int)payloadLength - chunkLength1;

            Assert.Equal((ushort)FfRequestVersion.V1, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet2, i2)));
            i2 += 2;
            // Request ID (uint64)
            Assert.NotEqual(new byte[8], packet2.Skip(i2).Take(8).ToArray());
            Assert.Equal(packet1.Skip(1).Take(8).ToArray(), packet2.Skip(1).Take(8).ToArray());
            i2 += 8;
            // Request length (uint32)
            Assert.Equal(payloadLength, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet2, i2)));
            i2 += 4;
            // Chunk offset (uint32)
            Assert.Equal((uint)chunkLength1, (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(packet2, i2)));
            i2 += 4;
            // Chunk length (uint16)
            Assert.Equal(chunkLength2, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet2, i2)));
            i2 += 2;

            // EOL option
            Assert.Equal((byte)FfRequestOption.Type.TYPE_EOL, packet2[i2++]);
            // Option length (uint16)
            Assert.Equal(0, (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(packet2, i2)));
            i2 += 2;

            // Payload
            Assert.Equal(
                Encoding.UTF8.GetBytes(requestPayload).Skip(chunkLength1).ToArray(),
                packet2.Skip(i2).Take(chunkLength2).ToArray()
            );
        }
    }
}
