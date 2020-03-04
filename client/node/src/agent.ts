import dns from "dns";
import dgram from "dgram";
import net from "net";
import createDebug from "debug";
import { Agent, ClientRequest, RequestOptions } from "agent-base";
import FfClient, { FfClientOptions } from "./client";
import { TcpToFfSocket, TcpToFfSocketOptions } from "./tcp-to-ff-socket";

const debug = createDebug("ff-client");

export interface FfClientAgentOptions
  extends FfClientOptions,
    TcpToFfSocketOptions {}

export class FfClientAgent extends Agent {
  public readonly ffClient: FfClient;

  constructor(private readonly options: FfClientAgentOptions) {
    super();
    this.ffClient = new FfClient(options);
  }

  /**
   * Creates a UDP socket to the proxy.
   */
  async callback(
    req: ClientRequest,
    opts: RequestOptions
  ): Promise<net.Socket> {
    const socket = new TcpToFfSocket(this.ffClient, {
      https: this.options.https,
      mockResponse: this.options.mockResponse
    });

    return socket as net.Socket;
  }
}
