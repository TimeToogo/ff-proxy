import { FfClient, FfRequestOptions } from "../src/client";
import { TcpToFfSocket } from "../src";

describe("TcpToFfSocket", () => {
  it("Calls FfClient.sendRequest with correct options", async () => {
    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(new Promise(() => {}))
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true
    });

    const request = "GET / HTTP/1.1\nHost: google.com\n\n";

    socket.write(request);

    expect(mockClient.sendRequest).toHaveBeenCalledTimes(1);
    expect(mockClient.sendRequest).toBeCalledWith({
      https: true,
      request: Buffer.from(request, "utf-8")
    } as FfRequestOptions);
  });

  it("Write callback is called after successful FfClient request", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true
    });

    let callback = jest.fn();

    socket.write("GET / HTTP/1.1\nHost: google.com\n\n", undefined, callback);

    resolveSendRequest!();
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(callback).toBeCalledTimes(1);
    expect(callback).toBeCalledWith();
  });

  it("Passes FfClient error to callback", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true
    });

    let callback = jest.fn();

    socket._write("GET / HTTP/1.1\nHost: google.com\n\n", "utf-8", callback);

    rejectSendRequest!(new Error("some error"));
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(callback).toBeCalledTimes(1);
    expect(callback).toBeCalledWith(new Error("some error"));
  });

  it("Hangs up socket without mockResponse", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true
    });

    const socketEvents = {
      data: [] as any[],
      ended: false
    };

    socket.addListener("data", chunk => {
      socketEvents.data.push(chunk);
    });

    socket.addListener("end", () => {
      socketEvents.ended = true;
    });

    socket.write("GET / HTTP/1.1\nHost: google.com\n\n");

    resolveSendRequest!();
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(socketEvents.data).toEqual([]);
    expect(socketEvents.ended).toEqual(true);
  });

  it("Sends correct mockResponse with string", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true,
      mockResponse: "HTTP/1.1 200 OK\n\n\n"
    });

    const dataCallback = jest.fn();
    socket.addListener("data",dataCallback);

    socket.write("GET / HTTP/1.1\nHost: google.com\n\n");
    socket.bufferSize

    resolveSendRequest!();
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(dataCallback).toBeCalledTimes(1);
    expect(dataCallback).toBeCalledWith(Buffer.from("HTTP/1.1 200 OK\n\n\n"));
  });

  it("Sends correct mockResponse with Buffer", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true,
      mockResponse: Buffer.from("HTTP/1.1 200 OK\n\n\n")
    });

    const dataCallback = jest.fn();
    socket.addListener("data",dataCallback);

    socket.write("GET / HTTP/1.1\nHost: google.com\n\n");

    resolveSendRequest!();
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(dataCallback).toBeCalledTimes(1);
    expect(dataCallback).toBeCalledWith(Buffer.from("HTTP/1.1 200 OK\n\n\n"));
  });

  it("Sends correct mockResponse with number (status code)", async () => {
    let resolveSendRequest: Function, rejectSendRequest: Function;
    const promise = new Promise((res, rej) => {
      resolveSendRequest = res;
      rejectSendRequest = rej;
    });

    const mockClient = ({
      sendRequest: jest.fn().mockReturnValue(promise)
    } as any) as FfClient;

    const socket = new TcpToFfSocket(mockClient, {
      https: true,
      mockResponse: 201
    });

    const dataCallback = jest.fn();
    socket.addListener("data",dataCallback);

    socket.write("GET / HTTP/1.1\nHost: google.com\n\n");

    resolveSendRequest!();
    // Wait for event handlers to fire
    await new Promise(resolve => setTimeout(resolve, 1));

    expect(dataCallback).toBeCalledTimes(1);
    expect(dataCallback).toBeCalledWith(Buffer.from("HTTP/1.1 201 Created\n\n\n"));
  });
});
