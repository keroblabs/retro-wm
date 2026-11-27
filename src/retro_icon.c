#include "retro_wm.h"

#include <string.h>

static bool valid_canvas(const wm_canvas *canvas)
{
    const wm_framebuffer *fb = canvas->framebuffer;
    if (fb->pixels == NULL || fb->width <= 0 || fb->height <= 0 ||
        fb->stride_pixels < (size_t)fb->width ||
        (size_t)fb->height > SIZE_MAX / fb->stride_pixels ||
        fb->stride_pixels > SIZE_MAX / sizeof(wm_pixel) / (size_t)fb->height ||
        canvas->clip.width < 0 || canvas->clip.height < 0) {
        return false;
    }
    return true;
}

static wm_color rgb565_to_colour(uint16_t pixel)
{
    uint32_t red = (pixel >> 11) & 0x1f;
    uint32_t green = (pixel >> 5) & 0x3f;
    uint32_t blue = pixel & 0x1f;
    red = (red << 3) | (red >> 2);
    green = (green << 2) | (green >> 4);
    blue = (blue << 3) | (blue >> 2);
    return WM_RGB(red, green, blue);
}

wm_status wm_canvas_icon(wm_canvas *canvas, int32_t x, int32_t y,
                         const icon_t *icon, wm_color foreground)
{
    if (canvas == NULL || canvas->framebuffer == NULL || icon == NULL ||
        icon->icon_pixels == NULL || icon->width == 0 || icon->height == 0 ||
        !valid_canvas(canvas)) {
        return WM_ERROR_ARGUMENT;
    }

    size_t row_bytes = 0;
    if (icon->pixel_type == WM_ICON_PIXEL_1BPP) {
        row_bytes = ((size_t)icon->width + 7u) / 8u;
    } else if (icon->pixel_type == WM_ICON_PIXEL_RGB565) {
        if (icon->colour_key != WM_ICON_COLOUR_KEY_NONE &&
            icon->colour_key > UINT16_MAX) return WM_ERROR_ARGUMENT;
    } else if (icon->pixel_type == WM_ICON_PIXEL_RGB888) {
        if (icon->colour_key != WM_ICON_COLOUR_KEY_NONE &&
            icon->colour_key > UINT32_C(0x00ffffff)) return WM_ERROR_ARGUMENT;
    } else {
        return WM_ERROR_ARGUMENT;
    }

    const wm_framebuffer *fb = canvas->framebuffer;
    int64_t left = (int64_t)canvas->origin_x + x;
    int64_t top = (int64_t)canvas->origin_y + y;
    int64_t right = left + icon->width;
    int64_t bottom = top + icon->height;
    int64_t clip_right = (int64_t)canvas->clip.x + canvas->clip.width;
    int64_t clip_bottom = (int64_t)canvas->clip.y + canvas->clip.height;
    if (left < canvas->clip.x) left = canvas->clip.x;
    if (top < canvas->clip.y) top = canvas->clip.y;
    if (right > clip_right) right = clip_right;
    if (bottom > clip_bottom) bottom = clip_bottom;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > fb->width) right = fb->width;
    if (bottom > fb->height) bottom = fb->height;
    if (left >= right || top >= bottom) return WM_OK;

    for (int64_t dest_y = top; dest_y < bottom; ++dest_y) {
        size_t source_y = (size_t)(dest_y - ((int64_t)canvas->origin_y + y));
        wm_pixel *dest = fb->pixels + (size_t)dest_y * fb->stride_pixels;
        for (int64_t dest_x = left; dest_x < right; ++dest_x) {
            size_t source_x = (size_t)(dest_x - ((int64_t)canvas->origin_x + x));
            bool opaque = true;
            wm_color colour = foreground;
            if (icon->pixel_type == WM_ICON_PIXEL_1BPP) {
                const uint8_t *bytes = icon->icon_pixels;
                uint8_t byte = bytes[source_y * row_bytes + source_x / 8u];
                opaque = ((byte >> (7u - (source_x % 8u))) & 1u) != 0;
            } else if (icon->pixel_type == WM_ICON_PIXEL_RGB565) {
                uint16_t value;
                size_t index = source_y * (size_t)icon->width + source_x;
                memcpy(&value, (const uint8_t *)icon->icon_pixels + index * sizeof(value), sizeof(value));
                opaque = icon->colour_key == WM_ICON_COLOUR_KEY_NONE || value != icon->colour_key;
                colour = rgb565_to_colour(value);
#if WM_PIXEL_FORMAT == WM_PIXEL_FORMAT_RGB565
                if (opaque) dest[dest_x] = value;
#else
                if (opaque) dest[dest_x] = colour;
#endif
                continue;
            } else {
                uint32_t value;
                size_t index = source_y * (size_t)icon->width + source_x;
                memcpy(&value, (const uint8_t *)icon->icon_pixels + index * sizeof(value), sizeof(value));
                value &= UINT32_C(0x00ffffff);
                opaque = icon->colour_key == WM_ICON_COLOUR_KEY_NONE || value != icon->colour_key;
                colour = WM_RGB((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff);
            }
            if (opaque) dest[dest_x] = wm_color_to_pixel(colour);
        }
    }
    return WM_OK;
}
