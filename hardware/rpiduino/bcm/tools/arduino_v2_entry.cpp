#include <stdint.h>
#include <lvgl.h>
#include "xdos_corejobs.h"
#include "xdos_sdk_v1.h"

void setup();
void loop();
extern "C" __attribute__((weak)) void xdos_sketch_stop() {}
extern "C" __attribute__((weak)) bool xdos_sketch_can_stop() { return true; }

extern "C" int32_t xapp_entry(const XdosHostApiV1 *api);

namespace {
lv_timer_t *loopTimer;
int sketchSlot;
void (*hostLog)(void *, const char *);
void *hostContext;
bool closing;

void runLoop(lv_timer_t *)
{
    if (!closing) {
        loop();
        return;
    }
    if (!xdos_sketch_can_stop()) return;
    lv_timer_delete(loopTimer);
    loopTimer = nullptr;
    xdos_sketch_stop();
    xdos_v2_exit(sketchSlot);
}

void closeSketch(void *)
{
    closing = true;
}
}

extern "C" void rpiduino_xdos_log(const char *message)
{
    if (hostLog) hostLog(hostContext, message);
}

extern "C" int32_t xapp_entry(const XdosHostApiV1 *api)
{
    if (!api || api->abi_version != XDOS_ABI_VERSION)
        return -1;

    sketchSlot = xdos_v2_slot_of(reinterpret_cast<const void *>(xapp_entry));
    if (sketchSlot < 0)
        return -1;

    loopTimer = lv_timer_create(runLoop, 10, nullptr);
    if (!loopTimer)
        return -1;

    xdos_v2_register_close(sketchSlot, closeSketch, nullptr);
    if (api->struct_bytes >= __builtin_offsetof(XdosHostApiV1, show_lvgl_demo)) {
        hostLog = api->log;
        hostContext = api->context;
    }
    setup();
    return 0;
}
