import { TcpToFfSocket, FfClientAgent } from "../src";
import http from "http";

describe("FfClientAgent", () => {
  it("Creates TcpToFfSocket for HTTP request", async () => {
    const agent = new FfClientAgent({
      ipAddress: "mock",
      port: 0,
      mockResponse: 200,
    });

    const socket: TcpToFfSocket = (await agent.callback(
      {} as any,
      { secureEndpoint: false } as any
    )) as any;

    expect(socket).toBeInstanceOf(TcpToFfSocket);
    expect(socket.ffClient).toEqual(agent.ffClient);
    expect(socket.options.https).toEqual(false);
    expect(socket.options.mockResponse).toEqual(200);
  });

  it("Creates TcpToFfSocket for HTTPS request", async () => {
    const agent = new FfClientAgent({
      ipAddress: "mock",
      port: 0,
      mockResponse: 200,
    });

    const socket: TcpToFfSocket = (await agent.callback(
      {} as any,
      { secureEndpoint: true } as any
    )) as any;

    expect(socket).toBeInstanceOf(TcpToFfSocket);
    expect(socket.ffClient).toEqual(agent.ffClient);
    expect(socket.options.https).toEqual(true);
    expect(socket.options.mockResponse).toEqual(200);
  });

  it("can send basic HTTP request", async () => {
    const agent = new FfClientAgent({
      ipAddress: "127.0.0.1",
      port: 8080,
      mockResponse: 200,
    });

    const request = http.request({
      method: "GET",
      path: "/",
      host: "www.google.com",
      agent,
    });

    await new Promise((resolve) => request.end(resolve));
  });

  it("can send large HTTP request", async () => {
    const largePayload = Buffer.alloc(10000, "a");

    const agent = new FfClientAgent({
      ipAddress: "127.0.0.1",
      port: 8080,
      mockResponse: 200,
    });

    const request = http.request({
      method: "POST",
      path: "/",
      host: "www.google.com",
      agent,
    });

    await new Promise((resolve) => request.write(largePayload, resolve));
    request.end();

    expect(request.finished).toBe(true);
  });
});
