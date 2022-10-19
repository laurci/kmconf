import struct from "./struct";

const HEADER_LENGTH = 1;

export enum PacketType {
    TEST = 0x00,
    READ_REQUEST = 0x01,
    READ_RESPONSE = 0x02,
};

export interface TestPacket {
    a: number;
    b: number;
    c: number;
};

export interface ReadRequestPacket {
    req_id: number;
    address: number;
};

export interface ReadResponsePacket {
    req_id: number;
    address: number;
    length: number;
};

const testStruct = struct<[a: number, b: number, c: number]>("III"); // 3 unsigned integers
const readRequestStruct = struct<[req_id: number, address: number]>("II"); // 2 unsigned integers
const readResponseStruct = struct<[req_id: number, address: number, length: number]>("III"); // 3 unsigned integers

const protocol = {
    test: {
        struct: testStruct,

        unpack(buffer: ArrayBuffer) {
            const [a, b, c] = testStruct.unpack_from(buffer, HEADER_LENGTH);
            return { a, b, c };
        }
    },
    readRequest: {
        struct: readRequestStruct,

        unpack(buffer: ArrayBuffer) {
            const [req_id, address] = readRequestStruct.unpack_from(buffer, HEADER_LENGTH);
            return { req_id, address };
        }
    },
    readResponse: {
        struct: readResponseStruct,

        pack(response: ReadResponsePacket) {
            return readResponseStruct.pack(response.req_id, response.address, response.length);
        }
    }
}

export default protocol;