using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;
using System.Linq;
using Microsoft.AspNetCore.Cryptography.KeyDerivation;

namespace FfClient
{
    public class FfClient
    {
        private const ushort MAX_PACKET_LENGTH = 1300;

        private readonly FfConfig config;

        private readonly RandomNumberGenerator rng;


        public FfClient(FfConfig config)
        {
            if (config == null)
            {
                throw new ArgumentNullException(nameof(config));
            }

            this.config = config;
            this.rng = RNGCryptoServiceProvider.Create();

            this.config.Validate();
        }

        public async Task SendRequest(HttpRequestMessage httpRequest)
        {
            if (httpRequest == null)
            {
                throw new ArgumentNullException(nameof(httpRequest));
            }

            var packets = await this.CreateRequestPackets(httpRequest);
            int totalSent = 0;

            Debug.WriteLine("Initialising UDP client");
            using (var udpClient = new UdpClient())
            {
                Debug.WriteLine($"Connecting to {this.config.IPAddress.ToString()}:{this.config.Port}");
                udpClient.Connect(this.config.IPAddress, this.config.Port);

                foreach (var packet in packets)
                {
                    await udpClient.SendAsync(packet.Packet, packet.Length);
                    totalSent += packet.Length;
                }

                Debug.WriteLine($"Finished sending {totalSent} bytes");
            }
        }

        internal async Task<List<UdpPacket>> CreateRequestPackets(HttpRequestMessage httpRequest)
        {
            Debug.WriteLine("Sending HTTP request using FfClient");

            var requestIdBytes = new byte[8];
            this.rng.GetBytes(requestIdBytes);

            var request = new FfRequest()
            {
                Version = FfRequestVersion.V1,
                RequestId = BitConverter.ToUInt64(requestIdBytes)
            };

            if (httpRequest.RequestUri.Scheme.ToLower() == "https")
            {
                request.Options.Add(new FfRequestOption()
                {
                    OptionType = FfRequestOption.Type.TYPE_HTTPS,
                    Value = new byte[] { 1 }
                });
            }

            byte[] httpMessage = await this.SerializeHttpMessage(httpRequest);

            if (!string.IsNullOrEmpty(this.config.PreSharedKey))
            {
                httpMessage = this.EncryptHttpMessage(httpMessage, request);
            }

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_EOL
            });

            request.Payload = httpMessage;

            return this.PacketiseRequest(request);
        }

        internal async Task<byte[]> SerializeHttpMessage(HttpRequestMessage httpRequest)
        {
            await this.NormaliseHttpMessage(httpRequest);

            var header = new StringBuilder();

            header.Append(httpRequest.Method.Method);
            header.Append(" ");
            header.Append(httpRequest.RequestUri.PathAndQuery);
            header.Append(" HTTP/1.1");
            header.AppendLine();

            var headers = httpRequest.Headers.ToList();

            if (httpRequest.Content?.Headers != null)
            {
                headers.AddRange(httpRequest.Content.Headers);
            }

            foreach (var httpHeader in headers)
            {
                foreach (var value in httpHeader.Value)
                {
                    header.Append(httpHeader.Key);
                    header.Append(": ");
                    header.Append(value);
                    header.AppendLine();
                }
            }

            header.AppendLine();

            Debug.WriteLine("Loading Http Content into buffer");
            byte[] httpContentBuffer = httpRequest.Content == null
                ? new byte[0]
                : await httpRequest.Content.ReadAsByteArrayAsync();

            int length = header.Length + httpContentBuffer.Length;

            byte[] message = new byte[length];

            Buffer.BlockCopy(
                Encoding.UTF8.GetBytes(header.ToString()),
                0,
                message,
                0,
                header.Length
            );

            Buffer.BlockCopy(
                httpContentBuffer,
                0,
                message,
                header.Length,
                httpContentBuffer.Length
            );

            Debug.WriteLine($"Finished serializing HTTP request ({message.Length} bytes)");

            return message;
        }

        internal async Task NormaliseHttpMessage(HttpRequestMessage httpRequest)
        {
            if (httpRequest.Headers.Host == null && httpRequest.RequestUri.Host != null)
            {
                httpRequest.Headers.Host = httpRequest.RequestUri.Host;
            }

            if (httpRequest.Content?.Headers != null)
            {
                httpRequest.Content.Headers.ContentLength = (await httpRequest.Content?.ReadAsByteArrayAsync()).Length;
            }
        }

        internal byte[] EncryptHttpMessage(byte[] httpMessage, FfRequest request)
        {
            Debug.WriteLine("Encrypting HTTP message using AES-256-GCM with PBKDF2");

            var salt = new byte[16];
            this.rng.GetBytes(salt);
            var derivedKey = KeyDerivation.Pbkdf2(
                password: this.config.PreSharedKey,
                salt: salt,
                prf: KeyDerivationPrf.HMACSHA256,
                iterationCount: this.config.Pbkdf2Iterations,
                numBytesRequested: 256 / 8
            );

            var iv = new byte[12];
            this.rng.GetBytes(iv);
            var tag = new byte[16];
            var cipherText = new byte[httpMessage.Length];


            using (var aesGcm = new AesGcm(derivedKey))
            {
                aesGcm.Encrypt(iv, httpMessage, cipherText, tag);
            }

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_ENCRYPTION_MODE,
                Value = new byte[] { (byte)FfEncryptionMode.AES_256_GCM }
            });

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_ENCRYPTION_IV,
                Value = iv
            });

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_ENCRYPTION_TAG,
                Value = tag
            });

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_KEY_DERIVE_MODE,
                Value = new byte[] { (byte)FfKeyDeriveMode.PBKDF2 }
            });

            request.Options.Add(new FfRequestOption()
            {
                OptionType = FfRequestOption.Type.TYPE_KEY_DERIVE_SALT,
                Value = salt
            });

            return cipherText;
        }

        internal List<UdpPacket> PacketiseRequest(FfRequest request)
        {
            var packets = new List<UdpPacket>();
            uint bytesLeft = (uint)request.Payload.Length;
            uint chunkOffset = 0;

            while (bytesLeft > 0)
            {
                var packet = new byte[MAX_PACKET_LENGTH];
                int i = 0;

                this.WriteNumber<ushort>(packet, (ushort)request.Version, ref i);
                this.WriteNumber<ulong>(packet, request.RequestId, ref i);
                this.WriteNumber<uint>(packet, (uint)request.Payload.Length, ref i);
                this.WriteNumber<uint>(packet, chunkOffset, ref i);
                // Defer writing chunk length (int16)
                int chunkLengthOffset = i;
                i += 2;

                if (chunkOffset == 0)
                {
                    foreach (var option in request.Options)
                    {
                        packet[i++] = (byte)option.OptionType;
                        var optionLength = (ushort)(option.Value?.Length ?? 0);
                        this.WriteNumber<ushort>(packet, optionLength, ref i);

                        if (optionLength != 0)
                        {
                            Buffer.BlockCopy(option.Value, 0, packet, i, optionLength);
                            i += optionLength;
                        }
                    }
                }
                else
                {
                    // Only first packet has options so write EOL option for non initial packets
                    packet[i++] = (byte)FfRequestOption.Type.TYPE_EOL;
                    this.WriteNumber<ushort>(packet, 0, ref i);
                }

                ushort chunkLength = (ushort)Math.Min(bytesLeft, MAX_PACKET_LENGTH - i);

                Buffer.BlockCopy(request.Payload, (int)chunkOffset, packet, i, chunkLength);
                i += chunkLength;

                // Write chunk length
                this.WriteNumber<ushort>(packet, chunkLength, ref chunkLengthOffset);

                bytesLeft -= chunkLength;
                chunkOffset += chunkLength;

                packets.Add(new UdpPacket()
                {
                    Packet = packet,
                    Length = i
                });
            }

            return packets;
        }

        internal void WriteNumber<TNumber>(byte[] dest, TNumber number, ref int count)
            where TNumber : struct
        {
            byte[] bytes;

            switch (number)
            {
                case ushort val:
                    bytes = BitConverter.GetBytes((ushort)IPAddress.HostToNetworkOrder(unchecked((short)val)));
                    break;

                case uint val:
                    bytes = BitConverter.GetBytes((uint)IPAddress.HostToNetworkOrder(unchecked((int)val)));
                    break;

                case ulong val:
                    bytes = BitConverter.GetBytes((ulong)IPAddress.HostToNetworkOrder(unchecked((long)val)));
                    break;

                default:
                    throw new NotImplementedException($"Cannot call WriteNumber with type {typeof(TNumber).FullName}");
            }

            this.WriteBytes(dest, bytes, ref count);
        }

        internal void WriteBytes(byte[] dest, byte[] src, ref int count)
        {
            Buffer.BlockCopy(src, 0, dest, count, src.Length);
            count += src.Length;
        }
    }
}
