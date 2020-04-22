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
            TYPE_HTTPS = 4,
            TYPE_KEY_DERIVE_MODE = 5,
            TYPE_KEY_DERIVE_SALT = 6,
            TYPE_BREAK = 7,
            TYPE_TIMESTAMP = 8,
        }

        public FfRequestOption.Type OptionType { get; set; }
        public byte[] Value { get; set; }
        public int Length => this.Value?.Length ?? 0;
    }
}
