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
    Omit<TcpToFfSocketOptions, "https"> {}

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
    debug("Agent creating new TCP to FF socket adapter");

    const socket = new TcpToFfSocket(this.ffClient, {
      https: opts.secureEndpoint,
      mockResponse: this.options.mockResponse
    });

    return socket as net.Socket;
  }
}
