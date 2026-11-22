#pragma once

#include <stdint.h>

#if !defined(RPIDUINO_XDOS_XAPP_V2)
#error "Message Tunnel requires the XDOS XAPP V2 build target"
#endif

// Core 0 only. The channel has one receiver and a bounded FIFO.
// Returns 0 on success, 1 for full/empty, -1 for unavailable or invalid.
extern "C" {
int xdos_v2_ipc_listen(int slot, uint32_t channel);
int xdos_v2_ipc_send(int slot, uint32_t channel, uint32_t value);
int xdos_v2_ipc_receive(int slot, uint32_t *value);
}
