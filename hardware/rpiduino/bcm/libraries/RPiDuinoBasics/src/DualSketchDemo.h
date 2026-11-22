#pragma once

#include <stdint.h>
#include <lvgl.h>
#include <xdos_corejobs.h>
#include <RPiDuinoMessageTunnel.h>
#include <RPiDuinoXappV2.h>

class DualSketchDemo {
public:
    void begin(const char *title, int position)
    {
        slot = xdos_v2_slot_of(this);
        if (slot < 0) return;
        lv_obj_t *body = xdos_v2_window(slot, title, 300, 120);
        if (!body) return;
        lv_obj_align(lv_obj_get_parent(body), LV_ALIGN_CENTER, position, 0);
        label = lv_label_create(body);
        lv_obj_center(label);
        channel = 0x44534b00u + slot;  // One tunnel per sketch instance.
        listening = xdos_v2_ipc_listen(slot, channel) == 0;
        lv_label_set_text(label, listening ? "Waiting for first sample" : "IPC channel unavailable");
    }

    void tick(unsigned interval)
    {
        if (++ticks % interval != 0) return;
        const uint32_t actualCpu = xdos_this_core();
        const uint32_t sample = (actualCpu << 30) | (++count & CountMask);
        __atomic_store_n(&latest, sample, __ATOMIC_RELEASE);
    }

    void service()
    {
        if (!listening) return;
        const uint32_t sample = __atomic_load_n(&latest, __ATOMIC_ACQUIRE);
        if (sample != sent && xdos_v2_ipc_send(slot, channel, sample) == 0)
            sent = sample;

        uint32_t received;
        if (xdos_v2_ipc_receive(slot, &received) != 0) return;
#if RPIDUINO_SKETCH_CPU == -1
        const char *selected = "Auto";
#elif RPIDUINO_SKETCH_CPU == 2
        const char *selected = "CPU 2";
#elif RPIDUINO_SKETCH_CPU == 3
        const char *selected = "CPU 3";
#else
        const char *selected = "CPU 0";
#endif
        const unsigned observed = received >> 30;
        const unsigned total = received & CountMask;
        lv_label_set_text_fmt(label, "%s -> CPU %u | Count: %u", selected, observed, total);
        if (total == 1 || total == 5 || total == 10)
            rpiduino_xdos_log(lv_label_get_text(label));
    }

private:
    static constexpr uint32_t CountMask = 0x3fffffffu;
    lv_obj_t *label;
    uint32_t channel;
    uint32_t latest;
    uint32_t sent;
    uint32_t count;
    unsigned ticks;
    int slot;
    bool listening;
};
