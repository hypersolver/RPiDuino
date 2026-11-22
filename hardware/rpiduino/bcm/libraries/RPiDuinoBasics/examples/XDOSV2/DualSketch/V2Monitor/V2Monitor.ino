#include <Arduino.h>
#include <RPiDuinoXappV2.h>
#include <lvgl.h>
#include <xdos_corejobs.h>

static lv_obj_t *label;
static unsigned ticks;

void setup()
{
    const int slot = xdos_v2_slot_of(&ticks);
    lv_obj_t *body = xdos_v2_window(slot, "Dual Sketch: Monitor", 240, 120);
    if (!body) return;
    lv_obj_align(lv_obj_get_parent(body), LV_ALIGN_CENTER, 140, 0);
    label = lv_label_create(body);
    lv_obj_center(label);
    lv_label_set_text(label, "Monitor: 0");
}

void loop()
{
    if (!label || ++ticks % 80 != 0) return;
    lv_label_set_text_fmt(label, "Monitor: %u", ticks / 80);
    if (ticks == 80) rpiduino_xdos_log("V2Monitor: first UI update");
}
