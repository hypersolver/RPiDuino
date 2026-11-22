#include <Arduino.h>
#include <RPiDuinoXappV2.h>
#include <lvgl.h>
#include <xdos_corejobs.h>

static lv_obj_t *label;
static unsigned ticks;

void setup()
{
    const int slot = xdos_v2_slot_of(&ticks);
    lv_obj_t *window = xdos_v2_window(slot, "Arduino counter", 240, 120);
    if (window) label = lv_label_create(window);
}

void loop()
{
    if (label && ++ticks == 50) rpiduino_xdos_log("V2Counter: first UI update");
    if (label && ticks % 50 == 0)
        lv_label_set_text_fmt(label, "Counter: %u", ticks / 50);
}
