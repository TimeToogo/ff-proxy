import dns from "dns";
import dgram from "dgram";
import net from "net";
import createDebug from "debug";
import { Agent, ClientRequest, RequestOptions } from "agent-base";

const debug = createDebug("ff-client");

export interface FfClientAgentOptions {
  ipAddress: string;
  port: number;
}

export default class FfClientAgent extends Agent {
  constructor(private readonly options: FfClientAgentOptions) {
    super();
  }

  /**
   * Creates a UDP socket to the proxy.
   */
  async callback(
    req: ClientRequest,
    opts: RequestOptions
  ): Promise<net.Socket> {
    const socket = dgram.createSocket({
        type: "udp4",
        reuseAddr: true
    });

    socket.bind(this.options.port, this.options.ipAddress);

    socket.
    return socket;
  }
}
