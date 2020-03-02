// @see ../../../../src/request.h

export enum FfRequestVersion {
  VERSION_RAW = -1,
  VERSION_1 = 1
}

export enum FfRequestOptionType {
  EOL = 0,
  ENCRYPTION_MODE = 1,
  ENCRYPTION_IV = 2,
  ENCRYPTION_TAG = 3,
  HTTPS = 4
}

export enum FfEncryptionMode {
  AES_256_GCM = 1
}

export interface FfRequestOption {
  type: FfRequestOptionType;
  length: number;
  value: Uint8Array;
}

export interface FfRequest {
  requestId: Uint8Array;
  payload: Uint8Array;
  options: FfRequestOption[];
}
