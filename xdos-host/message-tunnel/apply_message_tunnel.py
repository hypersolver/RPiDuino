"""Integrate the bounded XAPP IPC inbox into a prepared MC4FQ runtime."""

import shutil
import sys
from pathlib import Path


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f"Unexpected MC4FQ runtime source near {before!r}")
    return source.replace(before, after, 1)


def main():
    runtime = Path(sys.argv[1])
    source = runtime.read_text(encoding="utf-8")
    source = replace_once(source, '#include "xdos_corejobs.h"',
                          '#include "xdos_corejobs.h"\n#include "xapp_ipc.h"')
    source = replace_once(source, '} // namespace', """XappIpc messageTunnel;
XappIpc::Endpoint endpoints[XappIpc::Slots];
uint32_t subscriptions[XappIpc::Slots];

bool CanUseTunnel(int slot)
{
    return xdos_this_core() == 0 && ValidSlot(slot)
        && !instances[slot].closing && !instances[slot].exitPending;
}

void CloseTunnel(unsigned slot)
{
    subscriptions[slot] = 0;
    messageTunnel.Deactivate(endpoints[slot]);
}

} // namespace

extern "C" int xdos_v2_ipc_listen(int slot, uint32_t channel)
{
    if (!CanUseTunnel(slot) || !channel) return -1;
    for (unsigned i = 0; i < XappIpc::Slots; ++i)
        if (subscriptions[i] == channel && i != static_cast<unsigned>(slot))
            return -1;
    if (subscriptions[slot] != 0 && subscriptions[slot] != channel)
        return -1;
    subscriptions[slot] = channel;
    return 0;
}

extern "C" int xdos_v2_ipc_send(int slot, uint32_t channel, uint32_t value)
{
    if (!CanUseTunnel(slot) || !channel) return -1;
    for (unsigned i = 0; i < XappIpc::Slots; ++i) {
        if (subscriptions[i] != channel) continue;
        if (!CanUseTunnel(i)) return -1;
        const auto result = messageTunnel.Send(endpoints[slot], endpoints[i], &value, sizeof value);
        if (result == XappIpc::Ok) return 0;
        return result == XappIpc::Full ? 1 : -1;
    }
    return -1;
}

extern "C" int xdos_v2_ipc_receive(int slot, uint32_t *value)
{
    if (!CanUseTunnel(slot) || !value || !subscriptions[slot]) return -1;
    XappIpc::Message message;
    const auto result = messageTunnel.Receive(endpoints[slot], message);
    if (result == XappIpc::Empty) return 1;
    if (result != XappIpc::Ok || message.size != sizeof *value) return -1;
    memcpy(value, message.data, sizeof *value);
    return 0;
}""")
    source = replace_once(source, '    const XappLoaderV2::Result r =',
                          '    endpoints[slot] = messageTunnel.Activate(slot);\n'
                          '    const XappLoaderV2::Result r =')
    source = replace_once(source, '        ReleaseMemory(static_cast<unsigned>(slot));',
                          '        CloseTunnel(slot);\n        ReleaseMemory(static_cast<unsigned>(slot));')
    source = replace_once(source, '        ReleaseMemory(i);',
                          '        CloseTunnel(i);\n        ReleaseMemory(i);')
    shutil.copyfile(Path(__file__).with_name("xapp_ipc.h"), runtime.with_name("xapp_ipc.h"))
    runtime.write_text(source, encoding="utf-8")


if __name__ == "__main__":
    main()
