import net from "net";
import stream from "stream";
import FfClient from "./client";

export interface TcpToFfSocketOptions {
  https: boolean;
  mockResponse?: string;
}

const DEFAULT_MOCK_RESPONSE = 'HTTP/1.1 200 OK\n\n\n'

export class TcpToFfSocket extends stream.Duplex {
  bufferSize: number = 0;
  bytesRead: number = 0;
  bytesWritten: number = 0;
  connecting: boolean = false;
  localAddress: string = "";
  localPort: number = 0;
  remoteAddress?: string | undefined;
  remoteFamily?: string | undefined;
  remotePort?: number | undefined;

  constructor(
    private readonly ffClient: FfClient,
    private readonly options: TcpToFfSocketOptions
  ) {
    super();
  }

  connect(): this {
    throw new Error("Method not implemented.");
  }

  setTimeout(timeout: number, callback?: (() => void) | undefined): this {
    return this;
  }

  setNoDelay(noDelay?: boolean | undefined): this {
    throw new Error("Method not implemented.");
  }

  setKeepAlive(
    enable?: boolean | undefined,
    initialDelay?: number | undefined
  ): this {
    throw new Error("Method not implemented.");
  }

  address(): string | net.AddressInfo {
    throw new Error("Method not implemented.");
  }

  unref(): this {
    throw new Error("Method not implemented.");
  }

  ref(): this {
    throw new Error("Method not implemented.");
  }

  _write(
    chunk: any,
    encoding: string,
    callback: (error?: Error | null) => void
  ): void {
    if (typeof chunk === "string") {
      chunk = Buffer.from(chunk, encoding as any);
    }

    this.ffClient
      .sendRequest({
        request: chunk,
        https: this.options.https
      })
      .then(() => {
        callback();

        this.emit("data", Buffer.from(this.options.mockResponse || DEFAULT_MOCK_RESPONSE));
        this.emit("end");
      })
      .catch(e => callback(e));
  }

  _read(size: number): void {}
}
