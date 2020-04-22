import { FfClient } from "../src/client";
import {
  FfRequestVersion,
  FfRequestOptionType,
  FfEncryptionMode,
} from "../src/request";

describe("FfClient", () => {
  it("Creates correct packets for large request", async () => {
    const client = new FfClient({
      ipAddress: "mock",
      port: 0,
    });

    const request = new Uint8Array(2000);
    for (let i = 0; i < request.length; i++) {
      request[i] = i % 255;
    }

    const packets = await client._createRequestPackets({
      https: true,
      request,
    });

    expect(packets).toHaveLength(2);

    // ==== Packet 1 =====

    expect(packets[0].length).toEqual(1300);

    let ptr = 0;

    // Version (int16)
    expect(packets[0].payload[ptr++]).toEqual(0);
    expect(packets[0].payload[ptr++]).toEqual(FfRequestVersion.VERSION_1);

    // Request ID (int64)
    const packet1RequestId = packets[0].payload.slice(ptr, ptr + 8);
    ptr += 8;

    const payloadOptionsLength = 11 + 4 + 3; // Timestamp option + HTTPS option + EOL option

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
    ).toEqual(1277);

    // Break Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.BREAK);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    // HTTPS Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.HTTPS);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(1);
    expect(packets[0].payload[ptr++]).toEqual(1);

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

    for (let i = 0; i < 1277 - payloadOptionsLength; i++) {
      expect(packets[0].payload[ptr++]).toEqual(request[i]);
    }

    // ==== Packet 2 =====

    expect(packets[1].length).toEqual(764);

    ptr = 0;

    // Version (int16)
    expect(packets[1].payload[ptr++]).toEqual(0);
    expect(packets[1].payload[ptr++]).toEqual(FfRequestVersion.VERSION_1);

    // Request ID (int64)
    const packet2RequestId = packets[1].payload.slice(ptr, ptr + 8);
    expect(packet1RequestId).toEqual(packet2RequestId);
    ptr += 8;

    // Total Length (int32)
    expect(
      (packets[1].payload[ptr++] << 24) +
        (packets[1].payload[ptr++] << 16) +
        (packets[1].payload[ptr++] << 8) +
        (packets[1].payload[ptr++] << 0)
    ).toEqual(request.length + payloadOptionsLength);

    // Chunk Offset (int32)
    expect(
      (packets[1].payload[ptr++] << 24) +
        (packets[1].payload[ptr++] << 16) +
        (packets[1].payload[ptr++] << 8) +
        (packets[1].payload[ptr++] << 0)
    ).toEqual(1277);

    // Chunk Length (int16)
    expect(
      (packets[1].payload[ptr++] << 8) + (packets[1].payload[ptr++] << 0)
    ).toEqual(2000 - (1277 - payloadOptionsLength));

    // EOL Option
    expect(packets[1].payload[ptr++]).toEqual(FfRequestOptionType.EOL);
    // Option length (int16)
    expect(
      (packets[1].payload[ptr++] << 8) + (packets[1].payload[ptr++] << 0)
    ).toEqual(0);

    for (let i = 1277 - payloadOptionsLength; i < 2000; i++) {
      expect(packets[1].payload[ptr++]).toEqual(request[i]);
    }
  });
});
