#include <Arduino.h>
#include <RPiDuinoXappV2.h>
#include <RPiDuinoMessageTunnel.h>
#include <lvgl.h>
#include <xdos_corejobs.h>

constexpr uint32_t Channel = 0x52504931;  // RPI1
static int slot;
static lv_obj_t *label;
static bool listening;
static unsigned updates;

void setup()
{
    slot = xdos_v2_slot_of(&slot);
    lv_obj_t *window = xdos_v2_window(slot, "Arduino IPC display", 280, 120);
    if (!window) return;
    label = lv_label_create(window);
    listening = xdos_v2_ipc_listen(slot, Channel) == 0;
    lv_label_set_text(label, listening ? "Waiting for Arduino sender" : "Channel already in use");
}

void loop()
{
    if (!listening) return;
    uint32_t sample;
    // Drain at most the full inbox depth in one LVGL callback.
    for (unsigned i = 0; i < 8 && xdos_v2_ipc_receive(slot, &sample) == 0; ++i) {
        lv_label_set_text_fmt(label, "Arduino sample: %u", sample);
        if (++updates == 1 || updates == 5)
            rpiduino_xdos_log(lv_label_get_text(label));
    }
}
