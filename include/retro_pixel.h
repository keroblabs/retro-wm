#ifndef RETRO_PIXEL_H
#define RETRO_PIXEL_H

#include <stdint.h>

#define WM_PIXEL_FORMAT_RGB565 565
#define WM_PIXEL_FORMAT_RGB888 888

#ifndef WM_PIXEL_FORMAT
#define WM_PIXEL_FORMAT WM_PIXEL_FORMAT_RGB888
#endif

#if WM_PIXEL_FORMAT == WM_PIXEL_FORMAT_RGB565
typedef uint16_t wm_pixel;
#define WM_PIXEL_BITS 16
#elif WM_PIXEL_FORMAT == WM_PIXEL_FORMAT_RGB888
typedef uint32_t wm_pixel;
#define WM_PIXEL_BITS 32
#else
#error "WM_PIXEL_FORMAT must be WM_PIXEL_FORMAT_RGB565 or WM_PIXEL_FORMAT_RGB888"
#endif

typedef uint32_t wm_color;

#define WM_RGB(r, g, b) \
    (UINT32_C(0xff000000) | ((uint32_t)(r) << 16) | \
     ((uint32_t)(g) << 8) | (uint32_t)(b))

static inline wm_pixel wm_color_to_pixel(wm_color colour)
{
#if WM_PIXEL_FORMAT == WM_PIXEL_FORMAT_RGB565
    return (wm_pixel)(((colour & UINT32_C(0x00f80000)) >> 8) |
                      ((colour & UINT32_C(0x0000fc00)) >> 5) |
                      ((colour & UINT32_C(0x000000f8)) >> 3));
#else
    return colour;
#endif
}

#endif
