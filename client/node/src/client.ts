import dgram from "dgram";
import crypto from "crypto";
import { debug } from "./debug";
import {
  FfRequest,
  FfRequestOptionType,
  FfEncryptionMode,
  FfRequestVersion,
  FfKeyDeriveMode,
  FfRequestOption,
} from "./request";

export interface FfClientOptions {
  ipAddress: string;
  port: number;
  preSharedKey?: string;
  pbkdf2Iterations?: number;
}

export interface FfRequestOptions {
  request: string | Buffer | Uint8Array;
  https: boolean;
}

interface EncryptionResult {
  mode: FfEncryptionMode;
  iv: Uint8Array;
  tag: Uint8Array;
  ciphertext: Uint8Array;
  keyDeriveMode: FfKeyDeriveMode;
  salt: Uint8Array;
}

interface Packet {
  length: number;
  payload: Uint8Array;
}

const MAX_PACKET_LENGTH = 1300;
const OPTION_HEADER_LENGTH = 3;

export class FfClient {
  constructor(private readonly config: FfClientOptions) {
    if (this.config.preSharedKey && this.config.preSharedKey.length > 32) {
      throw new Error(`Pre-shared key cannot be longer than 32 chars`);
    }
  }

  /**
   * Sends a HTTP request payload over UDP to a FF server.
   */
  public sendRequest = async (options: FfRequestOptions): Promise<void> => {
    const packets = await this._createRequestPackets(options);

    debug(`Creating UDP socket`);
    const socket = dgram.createSocket({
      type: "udp4",
      reuseAddr: true,
    });

    debug(`Binding socket`);
    await new Promise((resolve) => socket.bind(undefined, undefined, resolve));

    let sent = 0;

    for (const packet of packets) {
      await new Promise((resolve, reject) =>
        socket.send(
          packet.payload,
          this.config.port,
          this.config.ipAddress,
          (err) => {
            err ? reject(err) : resolve();
          }
        )
      );
      sent += packet.length;
    }

    debug(`Sent ${sent} bytes to ${this.config.ipAddress}:${this.config.port}`);

    await new Promise((resolve) => socket.close(resolve));
    debug(`Socket closed`);
  };

  public _createRequestPackets = async (
    options: FfRequestOptions
  ): Promise<Packet[]> => {
    const request: FfRequest = {
      requestId: crypto.randomBytes(8),
      payload: Uint8Array.from(
        typeof options.request === "string"
          ? Buffer.from(options.request, "utf-8")
          : options.request
      ),
      options: [],
      secureOptions: [],
    };

    this._createSecureOptions(request, options);

    const secureOptions = this._serializeRequestOptions(request.secureOptions);
    const httpMessage = request.payload;

    request.payload = Buffer.concat([secureOptions, httpMessage]);

    if (this.config.preSharedKey) {
      const encryptionResult = await this._encryptPayload(request.payload);

      request.payload = encryptionResult.ciphertext;
      request.options.push({
        type: FfRequestOptionType.ENCRYPTION_MODE,
        length: 1,
        value: Buffer.of(encryptionResult.mode),
      });
      request.options.push({
        type: FfRequestOptionType.ENCRYPTION_IV,
        length: encryptionResult.iv.length,
        value: encryptionResult.iv,
      });
      request.options.push({
        type: FfRequestOptionType.ENCRYPTION_TAG,
        length: encryptionResult.tag.length,
        value: encryptionResult.tag,
      });
      request.options.push({
        type: FfRequestOptionType.KEY_DERIVE_MODE,
        length: 1,
        value: Buffer.of(encryptionResult.keyDeriveMode),
      });
      request.options.push({
        type: FfRequestOptionType.KEY_DERIVE_SALT,
        length: encryptionResult.salt.length,
        value: encryptionResult.salt,
      });
    }

    request.options.push({
      type: FfRequestOptionType.BREAK,
      length: 0,
      value: Buffer.alloc(0),
    });

    const packets: Packet[] = this._packetiseRequest(request);
    debug(`Packetised request into ${packets.length} packets`);

    return packets;
  };

  private _createSecureOptions = (
    request: FfRequest,
    options: FfRequestOptions
  ): void => {
    if (options.https) {
      request.secureOptions.push({
        type: FfRequestOptionType.HTTPS,
        length: 1,
        value: Buffer.of(1),
      });
    }

    request.secureOptions.push({
      type: FfRequestOptionType.TIMESTAMP,
      length: 8,
      value: this._getCurrentTimestampBuffer(),
    });

    request.secureOptions.push({
      type: FfRequestOptionType.EOL,
      length: 0,
      value: Buffer.alloc(0),
    });
  };

  private _serializeRequestOptions = (options: FfRequestOption[]): Buffer => {
    const length = options.reduce(
      (a, i) => OPTION_HEADER_LENGTH + i.length + a,
      0
    );
    const buff = Buffer.alloc(length);

    this._writeRequestOptions(buff, 0, options);

    return buff;
  };

  private _getCurrentTimestampBuffer = (): Buffer => {
    const buff = Buffer.alloc(8);
    let ptr = 0;
    const now = Math.floor(Date.now() / 1000);

    // This line is broken, hopefully node fixes 64bit math before this matters
    const nowHigh32 = now & (0xffffffff000000000 >> 32);
    const nowLow32 = now & 0xffffffff;

    for (let i = 0; i < 4; i++) {
      buff[ptr++] = (nowHigh32 >> (24 - i * 8)) & 0x000000ff;
    }

    for (let i = 0; i < 4; i++) {
      buff[ptr++] = (nowLow32 >> (24 - i * 8)) & 0x000000ff;
    }

    return buff;
  };

  public _encryptPayload = async (
    payload: Uint8Array
  ): Promise<EncryptionResult> => {
    debug(`Encrypting request payload`);

    if (!this.config.preSharedKey) {
      throw new Error(`Cannot encrypt payload without pre-shared key`);
    }

    const salt = Uint8Array.from(crypto.randomBytes(16));
    const derivedKey = await new Promise<Buffer>((resolve, reject) =>
      crypto.pbkdf2(
        this.config.preSharedKey!,
        salt,
        this.config.pbkdf2Iterations || 1000,
        256 / 8,
        "SHA256",
        (err, key) => {
          if (err) {
            reject(err);
          } else {
            resolve(key);
          }
        }
      )
    );

    const iv = Uint8Array.from(crypto.randomBytes(12));

    const cipher = crypto.createCipheriv("aes-256-gcm", derivedKey, iv);

    const ciphertext = Uint8Array.from(
      Buffer.concat([cipher.update(payload), cipher.final()])
    );
    const tag = Uint8Array.from(cipher.getAuthTag());

    debug(
      `Finished encrypting payload, (ciphertext length: ${ciphertext.length})`
    );

    return {
      mode: FfEncryptionMode.AES_256_GCM,
      iv,
      tag,
      ciphertext,
      keyDeriveMode: FfKeyDeriveMode.PBKDF2,
      salt,
    };
  };

  public _packetiseRequest = (request: FfRequest): Packet[] => {
    const packets = [] as Packet[];
    let bytesLeft = request.payload.length;
    let chunkOffset = 0;

    if (request.requestId.length !== 8) {
      throw new Error(`Request ID must be 8 bytes long`);
    }

    while (bytesLeft > 0) {
      const packetBuff = new Uint8Array(MAX_PACKET_LENGTH);
      let ptr = 0;
      let chunkLengthPtr = 0;

      // -- Request header --
      // Version (int16)
      packetBuff[ptr++] = 0;
      packetBuff[ptr++] = FfRequestVersion.VERSION_1;
      // Request ID (int64)
      for (let i = 0; i < 8; i++) {
        packetBuff[ptr++] = request.requestId[i];
      }
      // Total length (int32)
      for (let i = 0; i < 4; i++) {
        packetBuff[ptr++] =
          (request.payload.length >> (24 - i * 8)) & 0x000000ff;
      }
      // Chunk offset (int32)
      for (let i = 0; i < 4; i++) {
        packetBuff[ptr++] = (chunkOffset >> (24 - i * 8)) & 0x000000ff;
      }
      // Chunk length (int16), deferred
      chunkLengthPtr = ptr;
      ptr += 2;

      // -- Request Options --
      const requestOptions =
        chunkOffset === 0
          ? request.options
          : [
              {
                type: FfRequestOptionType.EOL,
                length: 0,
                value: new Uint8Array(0),
              },
            ];

      ptr = this._writeRequestOptions(packetBuff, ptr, requestOptions);

      const bytesLeftInPacket = MAX_PACKET_LENGTH - ptr;
      const chunkLength = Math.min(bytesLeft, bytesLeftInPacket);

      // -- Request payload --
      for (let i = 0; i < chunkLength; i++) {
        packetBuff[ptr++] = request.payload[chunkOffset + i];
      }

      // Request Header > Chunk length (int16)
      for (let i = 0; i < 2; i++) {
        packetBuff[chunkLengthPtr++] =
          (chunkLength >> (8 - i * 8)) & 0x000000ff;
      }

      packets.push({
        payload: packetBuff,
        length: ptr,
      });
      chunkOffset += chunkLength;
      bytesLeft -= chunkLength;
    }

    return packets;
  };

  private _writeRequestOptions = (
    packetBuff: Uint8Array,
    ptr: number,
    options: FfRequestOption[]
  ): number => {
    for (const option of options) {
      packetBuff[ptr++] = option.type;
      // Option length (int16)
      for (let i = 0; i < 2; i++) {
        packetBuff[ptr++] = (option.length >> (8 - i * 8)) & 0x000000ff;
      }
      for (let i = 0; i < option.length; i++) {
        packetBuff[ptr++] = option.value[i];
      }
    }

    return ptr;
  };
}
