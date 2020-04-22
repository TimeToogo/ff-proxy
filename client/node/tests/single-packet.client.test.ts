import { FfClient } from "../src/client";
import { FfRequestVersion, FfRequestOptionType } from "../src/request";

describe("FfClient", () => {
  it("Creates correct packet for small unencrypted request", async () => {
    const client = new FfClient({
      ipAddress: "mock",
      port: 0,
    });

    const request = "GET / HTTP/1.1\nHost: google.com\n\n";

    const packets = await client._createRequestPackets({
      https: false,
      request,
    });

    const payloadOptionsLength = 11 + 3; // Timestamp option + EOL option

    expect(packets).toHaveLength(1);

    expect(packets[0].length).toEqual(
      20 /* Header */ +
      3 /* Break option */ +
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

    // Break Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.BREAK);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    // Timestamp Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.TIMESTAMP);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(8);
    // Option value (int64)
    ptr += 4; // Skip first 4 bytes (node limitation)
    expect(
      (packets[0].payload[ptr++] << 24) +
        (packets[0].payload[ptr++] << 16) +
        (packets[0].payload[ptr++] << 8) +
        (packets[0].payload[ptr++] << 0)
    ).toBeCloseTo(Math.floor(Date.now() / 1000), -1);

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
