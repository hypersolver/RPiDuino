#pragma once

#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR < 2 || (LVGL_VERSION_MINOR == 2 && LVGL_VERSION_PATCH < 2)
#error "RPiduino LVGL RGB565 requires LVGL 9.2.2 or later in the 9.x series"
#endif

namespace RPiduino {

// The area uses inclusive coordinates. Consume pixels before returning.
using RGB565Write = void (*)(void *context, const lv_area_t &area,
                            const uint8_t *pixels, uint32_t strideBytes);

struct RGB565Display {
    uint16_t width;
    uint16_t height;
    void *buffer;
    void *secondBuffer;  // Optional, with the same capacity as buffer.
    uint32_t bufferBytes;  // Capacity of each buffer.
    RGB565Write write;
    void *context;
};

// Initialize LVGL first. Keep config and the buffers alive until display deletion.
lv_display_t *createRGB565Display(RGB565Display &config);

// Synchronous writer for a CPU-writable RGB565 framebuffer.
struct RGB565Framebuffer {
    uint8_t *pixels;
    size_t sizeBytes;
    uint32_t pitchBytes;
    uint16_t width;
    uint16_t height;
};

void writeRGB565Framebuffer(void *context, const lv_area_t &area,
                            const uint8_t *pixels, uint32_t strideBytes);

}  // namespace RPiduino
