#include <Arduino.h>
#include <RPiDuinoMessageTunnel.h>
#include <xdos_corejobs.h>

constexpr uint32_t Channel = 0x52504931;  // RPI1
static int slot;
static int worker = -1;
static uint32_t sample;
static unsigned ticks;

static int32_t nextSample(void *)
{
    return (sample + 1) & 0x7fffffff;
}

void setup()
{
    slot = xdos_v2_slot_of(&sample);
}

void loop()
{
    if (worker >= 0) {
        int32_t result;
        if (xdos_core_job_state(worker, &result) != 0 || !xdos_core_reclaim(worker))
            return;
        worker = -1;
        sample = result;
        xdos_v2_ipc_send(slot, Channel, sample);
    }
    if (++ticks % 50 == 0) {
        worker = xdos_core_lend_any(nextSample, nullptr);
        if (worker < 0) {
            sample = nextSample(nullptr);
            xdos_v2_ipc_send(slot, Channel, sample);
        }
    }
}

extern "C" bool xdos_sketch_can_stop()
{
    if (worker < 0) return true;
    // Defer unloading this module until the borrowed core has returned.
    return xdos_core_job_state(worker, nullptr) == 0 && xdos_core_reclaim(worker);
}
