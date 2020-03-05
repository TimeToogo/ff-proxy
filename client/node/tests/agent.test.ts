import FfClient, { FfRequestOptions } from "../src/client";
import { FfRequestVersion, FfRequestOptionType } from "../src/request";
import { TcpToFfSocket, FfClientAgent } from "../src";

describe("FfClientAgent", () => {
  it("Creates TcpToFfSocket for HTTP request", async () => {
    const agent = new FfClientAgent({
      ipAddress: "mock",
      port: 0,
      mockResponse: 200
    });

    const socket: TcpToFfSocket = await agent.callback(
      {} as any,
      { secureEndpoint: false } as any
    ) as any;

    expect(socket).toBeInstanceOf(TcpToFfSocket);
    expect(socket.ffClient).toEqual(agent.ffClient);
    expect(socket.options.https).toEqual(false);
    expect(socket.options.mockResponse).toEqual(200);
  });

  it("Creates TcpToFfSocket for HTTPS request", async () => {
    const agent = new FfClientAgent({
      ipAddress: "mock",
      port: 0,
      mockResponse: 200
    });

    const socket: TcpToFfSocket = await agent.callback(
      {} as any,
      { secureEndpoint: true } as any
    ) as any;

    expect(socket).toBeInstanceOf(TcpToFfSocket);
    expect(socket.ffClient).toEqual(agent.ffClient);
    expect(socket.options.https).toEqual(true);
    expect(socket.options.mockResponse).toEqual(200);
  });
});
