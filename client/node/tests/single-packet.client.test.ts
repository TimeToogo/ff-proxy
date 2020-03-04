import FfClient from "../src/client";
import {
  FfRequestVersion,
  FfRequestOptionType,
  FfEncryptionMode
} from "../src/request";

describe("FfClient", () => {
  it("Creates correct packet for small unencrypted request", async () => {
    const client = new FfClient({
      ipAddress: "mock",
      port: 0
    });

    const request = "GET / HTTP/1.1\nHost: google.com\n\n";

    const packets = await client._createRequestPackets({
      https: false,
      request
    });

    expect(packets).toHaveLength(1);

    expect(packets[0].length).toEqual(
      20 /* Header */ + 3 /* EOL Option */ + request.length
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
    ).toEqual(request.length);

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
    ).toEqual(request.length);

    // EOL Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.EOL);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    for (let i = 0; i < request.length; i++) {
      expect(Buffer.of(packets[0].payload[ptr++]).toString()[0]).toEqual(
        request[i]
      );
    }
  });
});
