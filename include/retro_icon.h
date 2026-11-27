#ifndef RETRO_ICON_H
#define RETRO_ICON_H

#include <stdint.h>

/*
 * Icon source formats are independent of WM_PIXEL_FORMAT, which configures
 * the destination framebuffer at compile time.
 *
 * RGB565: native uint16_t words, row-major, 5:6:5.
 * RGB888: native uint32_t words, row-major, 0x00RRGGBB.
 * 1BPP: MSB-first packed rows, each row rounded up to a byte; zero is
 *       transparent and one uses the caller's foreground colour.
 *
 * Colour keys are compared in the source format before conversion. The
 * WM_ICON_COLOUR_KEY_NONE sentinel disables colour-keying. For RGB565 keys
 * must fit in 16 bits; for RGB888 keys must fit in 24 bits. The key is ignored
 * for 1BPP resources.
 *
 * The descriptor does not include a backing-buffer length. Callers must keep
 * icon_pixels immutable and provide enough valid storage for width * height
 * source pixels (or ceil(width / 8) * height bytes for 1BPP). Colour-word
 * reads use memcpy, so source arrays need not be naturally aligned.
 */
typedef enum {
    WM_ICON_PIXEL_1BPP = 1,
    WM_ICON_PIXEL_RGB565 = 2,
    WM_ICON_PIXEL_RGB888 = 3
} wm_icon_pixel_type;

#define WM_ICON_COLOUR_KEY_NONE UINT32_MAX

typedef struct icon_t_ {
    uint16_t width;
    uint16_t height;
    uint8_t pixel_type;       /* wm_icon_pixel_type */
    uint32_t colour_key;      /* RGB565/RGB888; ignored for 1BPP */
    const void *icon_pixels;  /* const uint16_t*, uint32_t*, or packed bytes */
} icon_t;

#endif /* RETRO_ICON_H */
