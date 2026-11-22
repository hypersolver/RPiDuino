#include "RPiduino_LVGL_RGB565.h"

#include <string.h>

namespace RPiduino {
namespace {

void flush(lv_display_t *display, const lv_area_t *area, uint8_t *pixels)
{
    auto &config = *static_cast<RGB565Display *>(lv_display_get_driver_data(display));
    const auto *drawBuffer = lv_display_get_buf_active(display);
    config.write(config.context, *area, pixels, drawBuffer->header.stride);
    lv_display_flush_ready(display);
}

}  // namespace

lv_display_t *createRGB565Display(RGB565Display &config)
{
    if (!config.width || !config.height || !config.buffer || !config.write) {
        return nullptr;
    }

    constexpr auto format = LV_COLOR_FORMAT_RGB565;
    const auto stride = lv_draw_buf_width_to_stride(config.width, format);
    if (!stride || config.bufferBytes < stride ||
        lv_draw_buf_align(config.buffer, format) != config.buffer) {
        return nullptr;
    }

    if (config.secondBuffer) {
        const auto first = reinterpret_cast<uintptr_t>(config.buffer);
        const auto second = reinterpret_cast<uintptr_t>(config.secondBuffer);
        const auto distance = first < second ? second - first : first - second;
        if (distance < config.bufferBytes ||
            lv_draw_buf_align(config.secondBuffer, format) != config.secondBuffer) {
            return nullptr;
        }
    }

    auto *display = lv_display_create(config.width, config.height);
    if (!display) {
        return nullptr;
    }

    lv_display_set_color_format(display, format);
    lv_display_set_buffers(display, config.buffer, config.secondBuffer,
                           config.bufferBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_driver_data(display, &config);
    lv_display_set_flush_cb(display, flush);
    return display;
}

void writeRGB565Framebuffer(void *context, const lv_area_t &area,
                            const uint8_t *pixels, uint32_t strideBytes)
{
    const auto *fb = static_cast<const RGB565Framebuffer *>(context);
    if (!fb || !fb->pixels || !pixels || area.x1 < 0 || area.y1 < 0 ||
        area.x2 < area.x1 || area.y2 < area.y1 ||
        area.x2 >= fb->width || area.y2 >= fb->height ||
        fb->pitchBytes < 2u * fb->width) {
        return;
    }

    const auto rowBytes = 2u * static_cast<uint32_t>(area.x2 - area.x1 + 1);
    const auto end = static_cast<uint64_t>(area.y2) * fb->pitchBytes + 2u * (area.x2 + 1);
    const auto sourceEnd = static_cast<uint64_t>(area.y2 - area.y1) * strideBytes + rowBytes;
    if (strideBytes < rowBytes || end > fb->sizeBytes || sourceEnd > SIZE_MAX) {
        return;
    }

    for (int32_t y = area.y1; y <= area.y2; ++y) {
        const auto destOffset = static_cast<size_t>(y) * fb->pitchBytes + 2u * area.x1;
        const auto srcOffset = static_cast<size_t>(y - area.y1) * strideBytes;
        memcpy(fb->pixels + destOffset, pixels + srcOffset, rowBytes);
    }
}

}  // namespace RPiduino
