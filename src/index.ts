import ipc from "./lib/ipc";

async function main() {
    ipc.init();

    ipc.onReadAddress((address) => {
        return `reading address ${address} bla bla bla bla bla bla\nlalalaalala`;
    });
}

main();