import { FfClient } from "../src/client";
import {
  FfRequestVersion,
  FfRequestOptionType,
  FfEncryptionMode
} from "../src/request";

describe("FfClient", () => {
  it("Creates correct packets for large request", async () => {
    const client = new FfClient({
      ipAddress: "mock",
      port: 0
    });

    const request = new Uint8Array(2000);
    for (let i = 0; i < request.length; i++) {
      request[i] = i % 255;
    }

    const packets = await client._createRequestPackets({
      https: true,
      request
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
    ).toEqual(1273);

    // HTTPS Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.HTTPS);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(1);
    expect(packets[0].payload[ptr++]).toEqual(1);

    // EOL Option
    expect(packets[0].payload[ptr++]).toEqual(FfRequestOptionType.EOL);
    // Option length (int16)
    expect(
      (packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)
    ).toEqual(0);

    for (let i = 0; i < 1273; i++) {
      expect(packets[0].payload[ptr++]).toEqual(request[i]);
    }

    // ==== Packet 2 =====

    expect(packets[1].length).toEqual(750);

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
    ).toEqual(request.length);
    
    // Chunk Offset (int32)
    expect(
      (packets[1].payload[ptr++] << 24) +
        (packets[1].payload[ptr++] << 16) +
        (packets[1].payload[ptr++] << 8) +
        (packets[1].payload[ptr++] << 0)
    ).toEqual(1273);
    
    // Chunk Length (int16)
    expect(
      (packets[1].payload[ptr++] << 8) + (packets[1].payload[ptr++] << 0)
    ).toEqual(2000 - 1273);
    
    // EOL Option
    expect(packets[1].payload[ptr++]).toEqual(FfRequestOptionType.EOL);
    // Option length (int16)
    expect(
      (packets[1].payload[ptr++] << 8) + (packets[1].payload[ptr++] << 0)
    ).toEqual(0);
    
    for (let i = 1273; i < 2000; i++) {
      expect(packets[1].payload[ptr++]).toEqual(request[i]);
    }
  });
});
