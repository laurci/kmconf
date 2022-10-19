import fs from "fs/promises";
import fsSync from "fs";

import protocol, { PacketType } from "./protocol";
import { TextEncoder } from "util";

const IPC_FILE_PATH = "/dev/jscfgipc";

export type ReadAddressCallback = (address: number) => string | Promise<string>;

let readAddress: ReadAddressCallback | null = null;

const ipc = {
    init() {
        ipcTick(); // kickstart the event loop
    },
    onReadAddress(callback: ReadAddressCallback) {
        readAddress = callback;
    }
} as const;

const encoder = new TextEncoder();

async function handlePacket(type: number, data: ArrayBuffer) {
    switch (type) {
        case PacketType.TEST: {
            const packet = protocol.test.unpack(data);
            console.log("[in] test", packet);

            break;
        }
        case PacketType.READ_REQUEST: {
            const packet = protocol.readRequest.unpack(data);
            console.log("[in] read_request", packet);

            const text = readAddress ? await readAddress(packet.address) : "";
            const textBytes = encoder.encode(text);

            const result = new Uint8Array([
                PacketType.READ_RESPONSE,
                ...new Uint8Array(protocol.readResponse.pack({ req_id: packet.req_id, address: packet.address, length: textBytes.byteLength + 1 })),
                ...textBytes,
                ...new Uint8Array([0x00])
            ]);

            console.log("[out] read_response len =", result.byteLength);

            fsSync.writeFileSync(
                IPC_FILE_PATH,
                result
            );

            break;
        }
        default: {
            console.log("[in] unknown", type);
        }
    }
}

async function ipcTick() {
    const buffer = await fs.readFile(IPC_FILE_PATH);
    const data = new Uint8Array(buffer);
    const typeByte = data[0];

    handlePacket(typeByte, data.buffer);

    ipcTick();
}

export default ipc;