using System;
using System.Net;
using System.Net.Http;
using System.Threading.Tasks;

namespace FfClient
{
    public class FfRequestOption
    {
        public enum Type
        {
            TYPE_EOL = 0,
            TYPE_ENCRYPTION_MODE = 1,
            TYPE_ENCRYPTION_IV = 2,
            TYPE_ENCRYPTION_TAG = 3,
            TYPE_HTTPS = 4
        }

        public FfRequestOption.Type OptionType { get; set; }
        public byte[] Value { get; set; }
    }
}
