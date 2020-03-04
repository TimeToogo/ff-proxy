"use strict";
var __awaiter = (this && this.__awaiter) || function (thisArg, _arguments, P, generator) {
    return new (P || (P = Promise))(function (resolve, reject) {
        function fulfilled(value) { try { step(generator.next(value)); } catch (e) { reject(e); } }
        function rejected(value) { try { step(generator["throw"](value)); } catch (e) { reject(e); } }
        function step(result) { result.done ? resolve(result.value) : new P(function (resolve) { resolve(result.value); }).then(fulfilled, rejected); }
        step((generator = generator.apply(thisArg, _arguments || [])).next());
    });
};
var __generator = (this && this.__generator) || function (thisArg, body) {
    var _ = { label: 0, sent: function() { if (t[0] & 1) throw t[1]; return t[1]; }, trys: [], ops: [] }, f, y, t, g;
    return g = { next: verb(0), "throw": verb(1), "return": verb(2) }, typeof Symbol === "function" && (g[Symbol.iterator] = function() { return this; }), g;
    function verb(n) { return function (v) { return step([n, v]); }; }
    function step(op) {
        if (f) throw new TypeError("Generator is already executing.");
        while (_) try {
            if (f = 1, y && (t = op[0] & 2 ? y["return"] : op[0] ? y["throw"] || ((t = y["return"]) && t.call(y), 0) : y.next) && !(t = t.call(y, op[1])).done) return t;
            if (y = 0, t) op = [op[0] & 2, t.value];
            switch (op[0]) {
                case 0: case 1: t = op; break;
                case 4: _.label++; return { value: op[1], done: false };
                case 5: _.label++; y = op[1]; op = [0]; continue;
                case 7: op = _.ops.pop(); _.trys.pop(); continue;
                default:
                    if (!(t = _.trys, t = t.length > 0 && t[t.length - 1]) && (op[0] === 6 || op[0] === 2)) { _ = 0; continue; }
                    if (op[0] === 3 && (!t || (op[1] > t[0] && op[1] < t[3]))) { _.label = op[1]; break; }
                    if (op[0] === 6 && _.label < t[1]) { _.label = t[1]; t = op; break; }
                    if (t && _.label < t[2]) { _.label = t[2]; _.ops.push(op); break; }
                    if (t[2]) _.ops.pop();
                    _.trys.pop(); continue;
            }
            op = body.call(thisArg, _);
        } catch (e) { op = [6, e]; y = 0; } finally { f = t = 0; }
        if (op[0] & 5) throw op[1]; return { value: op[0] ? op[1] : void 0, done: true };
    }
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
var _this = this;
Object.defineProperty(exports, "__esModule", { value: true });
var client_1 = __importDefault(require("../src/client"));
var request_1 = require("../src/request");
describe("FfClient", function () {
    it("Creates correct packet for small unencrypted request", function () { return __awaiter(_this, void 0, void 0, function () {
        var client, request, packets, ptr, i;
        return __generator(this, function (_a) {
            switch (_a.label) {
                case 0:
                    client = new client_1.default({
                        ipAddress: "mock",
                        port: 0
                    });
                    request = "GET / HTTP/1.1\nHost: google.com\n\n";
                    return [4 /*yield*/, client._createRequestPackets({
                            https: false,
                            request: request
                        })];
                case 1:
                    packets = _a.sent();
                    expect(packets).toHaveLength(1);
                    expect(packets[0].length).toEqual(20 /* Header */ + 3 /* EOL Option */ + request.length);
                    ptr = 0;
                    // Version (int16)
                    expect(packets[0].payload[ptr++]).toEqual(0);
                    expect(packets[0].payload[ptr++]).toEqual(request_1.FfRequestVersion.VERSION_1);
                    // Request ID (int64)
                    ptr += 8;
                    // Total Length (int32)
                    expect((packets[0].payload[ptr++] << 24) +
                        (packets[0].payload[ptr++] << 16) +
                        (packets[0].payload[ptr++] << 8) +
                        (packets[0].payload[ptr++] << 0)).toEqual(request.length);
                    // Chunk Offset (int32)
                    expect((packets[0].payload[ptr++] << 24) +
                        (packets[0].payload[ptr++] << 16) +
                        (packets[0].payload[ptr++] << 8) +
                        (packets[0].payload[ptr++] << 0)).toEqual(0);
                    // Chunk Length (int16)
                    expect((packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)).toEqual(request.length);
                    // EOL Option
                    expect(packets[0].payload[ptr++]).toEqual(request_1.FfRequestOptionType.EOL);
                    // Option length (int16)
                    expect((packets[0].payload[ptr++] << 8) + (packets[0].payload[ptr++] << 0)).toEqual(0);
                    for (i = 0; i < request.length; i++) {
                        expect(Buffer.of(packets[0].payload[ptr++]).toString()[0]).toEqual(request[i]);
                    }
                    return [2 /*return*/];
            }
        });
    }); });
});
