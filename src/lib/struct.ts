const _struct = require("@aksel/structjs");

export type Endianess = "big" | "little" | "mixed";

export function endianess(): Endianess {
    let uInt32 = new Uint32Array([0x11223344]);
    let uInt8 = new Uint8Array(uInt32.buffer);

    if (uInt8[0] === 0x44) {
        return "little";
    } else if (uInt8[0] === 0x11) {
        return "big";
    } else {
        return "mixed";
    }
};


export default function struct<T extends any[]>(format: string, endian: Endianess = endianess()): Struct<T> {
    const final = `${endian == "little" ? "<" : ">"}${format}`;
    return _struct(final) as Struct<T>;
}

export interface Struct<T extends any[]> {
    pack(...values: T): ArrayBuffer;
    pack_into(buffer: ArrayBuffer, offset: number, ...values: T): void;
    unpack(buffer: ArrayBuffer): T;
    unpack_from(buffer: ArrayBuffer, offset: number): T;
    format: string;
    size: number;
}
