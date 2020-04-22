import { FfClient } from "../src/client";
import {
  FfRequestVersion,
  FfRequestOptionType,
  FfEncryptionMode,
  FfKeyDeriveMode,
} from "../src/request";

describe("FfClient", () => {
  it("Creates correct packet for small encrypted request", async () => {
    const client = new FfClient({
      ipAddress: "mock",
      port: 0,
      preSharedKey: "testkey",
    });

    const request = "GET / HTTP/1.1\nHost: google.com\n\n";

    const packets = await client._createRequestPackets({
      https: true,
      request,
    });

    const payloadOptionsLength = 11 + 4 + 3; // Timestamp option + HTTPS option + EOL option

    expect(packets).toHaveLength(1);

    expect(packets[0].length).toEqual(
      20 /* Header */ +
      4 /* Encryption Mode Option */ +
      19 /* Encryption IV Option */ +
      15 /* Encryption Tag Option */ +
      4 /* Key Derive Mode Option */ +
      19 /* Key Derive Salt Option */ +
      3 /* Break Option */ +
      payloadOptionsLength +
        request.length
    );

    let ptr = 0;

    // Version (int16)
    expect(packets[0].payload[ptr++]).toEqual(0);
    expect(packets[0].payload[ptr++]).toEqual(FfRequestVersion.VERSION_1);

    // Request ID (int64)
    ptr += 8;

    // Total Length (int32)
    expect(
      (packets[0].payload[ptr++] << 24) +
        (packets[0].payload[ptr++] << 16) +
        (packets[0].payload[ptr++] << 8) +
        (packets[0].payload[ptr++] << 0)
    ).toEqual(request.length + payloadOptionsLength);

    // Chunk Offset (int32)
    expect(
      (packets[0].payload[ptr++] << 24) +
        (packets[0].payload[ptr++] << 16) +
        (packets[0].payload[ptr++] << 8) +
        (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    // Chunk Length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(request.length + payloadOptionsLength);

    // Encryption Mode Option
    expect(packets[0].payload[ptr++]).toEqual(
      FfRequestOptionType.ENCRYPTION_MODE
    );
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(1);
    // Option payload
    expect(packets[0].payload[ptr++]).toEqual(FfEncryptionMode.AES_256_GCM);

    // Encryption IV Option
    expect(packets[0].payload[ptr++]).toEqual(
      FfRequestOptionType.ENCRYPTION_IV
    );
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(12);
    // Option payload
    expect(packets[0].payload.slice(ptr, ptr + 12)).not.toEqual(
      new Uint8Array(12)
    );
    ptr += 12;

    // Encryption Tag Option
    expect(packets[0].payload[ptr++]).toEqual(
      FfRequestOptionType.ENCRYPTION_TAG
    );
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(16);
    // Option payload
    expect(packets[0].payload.slice(ptr, ptr + 16)).not.toEqual(
      new Uint8Array(16)
    );
    ptr += 16;

    // Key Derive Mode Option
    expect(packets[0].payload[ptr++]).toEqual(
      FfRequestOptionType.KEY_DERIVE_MODE
    );
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(1);
    // Option payload
    expect(packets[0].payload[ptr++]).toEqual(FfKeyDeriveMode.PBKDF2);

    // Key Derive Salt Option
    expect(packets[0].payload[ptr++]).toEqual(
      FfRequestOptionType.KEY_DERIVE_SALT
    );
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(16);
    // Option payload
    expect(packets[0].payload.slice(ptr, ptr + 16)).not.toEqual(
      new Uint8Array(16)
    );
    ptr += 16;

    // Break Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.BREAK);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    expect(packets[0].payload.slice(ptr + payloadOptionsLength)).not.toEqual(
      Uint8Array.from(Buffer.from(request, "utf-8"))
    );
  });
});
