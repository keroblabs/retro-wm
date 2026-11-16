#ifndef RETRO_ICON_H
#define RETRO_ICON_H

#include <stdint.h>

/*
 * Icon source formats are independent of WM_PIXEL_FORMAT, which configures
 * the destination framebuffer at compile time. WM_PIXEL_FORMAT_RGB565 and
 * WM_PIXEL_FORMAT_RGB888 have values 565 and 888, not suitable for uint8_t.
 *
 * RGB565: one native uint16_t word per pixel (5:6:5).
 * RGB888: one native uint32_t word per pixel (0x00RRGGBB).
 * 1BPP: MSB-first packed rows, each row rounded up to a byte;
 *       zero is transparent, one uses the caller's chosen foreground colour.
 */
typedef enum {
    WM_ICON_PIXEL_1BPP = 1,
    WM_ICON_PIXEL_RGB565 = 2,
    WM_ICON_PIXEL_RGB888 = 3
} wm_icon_pixel_type;

/* No colour key for opaque RGB565/RGB888 resources. */
#define WM_ICON_COLOUR_KEY_NONE UINT32_MAX

typedef struct icon_t_ {
    uint16_t width;
    uint16_t height;
    uint8_t pixel_type;       /* wm_icon_pixel_type */
    uint32_t colour_key;      /* RGB565/RGB888; ignored for 1BPP (zero is clear) */
    const void *icon_pixels;  /* const uint16_t*, uint32_t*, or packed uint8_t* */
} icon_t;

#endif /* RETRO_ICON_H */
