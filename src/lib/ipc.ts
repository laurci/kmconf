import fs from "fs/promises";

const IPC_FILE_PATH = "/dev/jscfgipc";

const ipc = {

} as const;

async function ipcTick() {
    const data = await fs.readFile(IPC_FILE_PATH);
    console.log(data);
    ipcTick();
}

export function initIpc() {
    ipcTick();
}

export default ipc;