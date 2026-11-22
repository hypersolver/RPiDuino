#include <Arduino.h>
#include <RPiduino_LVGL_RGB565.h>

namespace {
constexpr uint16_t width = 160;
constexpr uint16_t height = 96;
constexpr uint16_t rows = 16;

alignas(LV_DRAW_BUF_ALIGN) uint8_t drawBuffer[LV_DRAW_BUF_STRIDE(width, LV_COLOR_FORMAT_RGB565) * rows];
uint8_t framebuffer[width * height * 2];

RPiduino::RGB565Framebuffer target = {framebuffer, sizeof(framebuffer), width * 2, width, height};
RPiduino::RGB565Display config = {
    width, height, drawBuffer, nullptr, sizeof(drawBuffer),
    RPiduino::writeRGB565Framebuffer, &target
};

lv_display_t *display;
}

void setup()
{
    lv_init();
    lv_tick_set_cb([]() -> uint32_t { return millis(); });
    display = RPiduino::createRGB565Display(config);
    if (!display) {
        return;
    }

    auto *label = lv_label_create(lv_display_get_screen_active(display));
    lv_label_set_text(label, "RPiduino RGB565");
    lv_obj_center(label);
}

void loop()
{
    if (display) {
        lv_timer_handler();
    }
    delay(5);
}
