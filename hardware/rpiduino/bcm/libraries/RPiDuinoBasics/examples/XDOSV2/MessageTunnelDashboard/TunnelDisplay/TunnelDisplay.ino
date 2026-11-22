#include <Arduino.h>
#include <RPiDuinoXappV2.h>
#include <RPiDuinoMessageTunnel.h>
#include <lvgl.h>
#include <xdos_corejobs.h>

constexpr uint32_t Channel = 0x52504931;  // RPI1
static int slot;
static lv_obj_t *label;
static lv_obj_t *gauge;
static bool listening;
static unsigned updates;

void setup()
{
    slot = xdos_v2_slot_of(&slot);
    lv_obj_t *window = xdos_v2_window(slot, "Message Tunnel Dashboard", 340, 150);
    if (!window) return;
    label = lv_label_create(window);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 16);
    gauge = lv_bar_create(window);
    lv_obj_set_size(gauge, 230, 18);
    lv_obj_align(gauge, LV_ALIGN_TOP_MID, 0, 54);
    lv_bar_set_range(gauge, 0, 99);
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
        lv_bar_set_value(gauge, sample % 100, LV_ANIM_OFF);
        if (++updates == 1 || updates == 5)
            rpiduino_xdos_log(lv_label_get_text(label));
    }
}
