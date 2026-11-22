#include <stdint.h>
#include <lvgl.h>
#include "xdos_corejobs.h"
#include "xdos_sdk_v1.h"

void setup();
void loop();
extern "C" __attribute__((weak)) void xdos_sketch_service() {}
extern "C" __attribute__((weak)) void xdos_sketch_stop() {}
extern "C" __attribute__((weak)) bool xdos_sketch_can_stop() { return true; }

extern "C" int32_t xapp_entry(const XdosHostApiV1 *api);

#if RPIDUINO_SKETCH_CPU != 0
extern "C" void __real_delay(unsigned long milliseconds);

extern "C" void __wrap_delay(unsigned long milliseconds)
{
    while (milliseconds) {
        const unsigned long step = milliseconds > 1000 ? 1000 : milliseconds;
        const int result = xdos_core_sleep_us(static_cast<uint32_t>(step * 1000));
        if (result < 0) {
            __real_delay(milliseconds);
            return;
        }
        if (result == 1) return;
        milliseconds -= step;
    }
}
#endif

extern "C" bool rpiduino_xdos_stop_requested()
{
#if RPIDUINO_SKETCH_CPU != 0
    return xdos_core_stop_requested() != 0;
#else
    return false;
#endif
}

namespace {
lv_timer_t *loopTimer;
int sketchSlot;
int worker = -1;
void (*hostLog)(void *, const char *);
void *hostContext;
bool closing;
bool loopPending;

#if RPIDUINO_SKETCH_CPU != 0
int32_t runWorker(void *)
{
    while (!xdos_core_stop_requested()) {
        if (__atomic_load_n(&loopPending, __ATOMIC_ACQUIRE)) {
            __atomic_store_n(&loopPending, false, __ATOMIC_RELAXED);
            loop();
        } else {
            asm volatile("wfe" ::: "memory");
        }
    }
    return 0;
}
#endif

void runLoop(lv_timer_t *)
{
    if (!closing) {
        if (worker < 0) {
            loop();
        } else {
            __atomic_store_n(&loopPending, true, __ATOMIC_RELEASE);
            asm volatile("dsb ishst\n\tsev" ::: "memory");
        }
        xdos_sketch_service();
        return;
    }
    if (worker >= 0) {
        if (xdos_core_job_state(worker, nullptr) != 0 || !xdos_core_reclaim(worker))
            return;
        worker = -1;
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
    if (worker >= 0) xdos_core_request_stop(worker);
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

    if (api->struct_bytes >= __builtin_offsetof(XdosHostApiV1, show_lvgl_demo)) {
        hostLog = api->log;
        hostContext = api->context;
    }
    loopTimer = lv_timer_create(runLoop, 10, nullptr);
    if (!loopTimer) return -1;

#if RPIDUINO_SKETCH_CPU != 0
#if RPIDUINO_SKETCH_CPU == -1
    for (int cpu = 3; cpu >= 2; --cpu) {
        if (xdos_core_lend(cpu, runWorker, nullptr)) {
            worker = cpu;
            break;
        }
    }
    if (worker < 0) rpiduino_xdos_log("RPiDuino Auto: no worker core available");
#else
    if (xdos_core_lend(RPIDUINO_SKETCH_CPU, runWorker, nullptr))
        worker = RPIDUINO_SKETCH_CPU;
#if RPIDUINO_SKETCH_CPU == 2
    if (worker < 0) rpiduino_xdos_log("RPiDuino CPU 2 is busy or unavailable");
#else
    if (worker < 0) rpiduino_xdos_log("RPiDuino CPU 3 is busy or unavailable");
#endif
#endif
    if (worker < 0) {
        lv_timer_delete(loopTimer);
        loopTimer = nullptr;
        return -1;
    }
#endif

    xdos_v2_register_close(sketchSlot, closeSketch, nullptr);
    setup();
    return 0;
}
