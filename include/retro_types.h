#ifndef RETRO_TYPES_H
#define RETRO_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint32_t wm_color;

#define WM_RGB(r, g, b) \
    (UINT32_C(0xff000000) | ((uint32_t)(r) << 16) | \
     ((uint32_t)(g) << 8) | (uint32_t)(b))

typedef enum {
    WM_OK,
    WM_ERROR_ARGUMENT,
    WM_ERROR_NOT_FOUND,
    WM_ERROR_CAPACITY
} wm_status;

typedef struct {
    int32_t x, y, width, height;
} wm_rect;

typedef struct {
    uint32_t *pixels;
    int32_t width, height;
    size_t stride_pixels;
} wm_framebuffer;

typedef struct {
    wm_framebuffer *framebuffer;
    wm_rect clip;
    int32_t origin_x, origin_y;
} wm_canvas;

typedef struct {
    wm_color desktop, desktop_alt, border;
    wm_color active_title, inactive_title, active_text, inactive_text;
    wm_color client, outline, outline_alt;
} wm_theme;

typedef enum {
    WM_EVENT_POINTER_MOVE,
    WM_EVENT_POINTER_DOWN,
    WM_EVENT_POINTER_UP,
    WM_EVENT_KEY_DOWN,
    WM_EVENT_CANCEL
} wm_event_type;

typedef enum {
    WM_BUTTON_NONE,
    WM_BUTTON_PRIMARY,
    WM_BUTTON_SECONDARY,
    WM_BUTTON_MIDDLE
} wm_button;

typedef enum {
    WM_KEY_NONE,
    WM_KEY_TAB,
    WM_KEY_ESCAPE,
    WM_KEY_F4,
    WM_KEY_W,
    WM_KEY_N,
    WM_KEY_C,
    WM_KEY_Q,
    WM_KEY_LEFT,
    WM_KEY_RIGHT,
    WM_KEY_UP,
    WM_KEY_DOWN,
    WM_KEY_ENTER,
    WM_KEY_SPACE,
    WM_KEY_F10
} wm_key;

enum {
    WM_MOD_SHIFT = 1u << 0,
    WM_MOD_CTRL = 1u << 1,
    WM_MOD_ALT = 1u << 2,
    WM_MOD_META = 1u << 3
};

typedef struct {
    wm_event_type type;
    int32_t x, y;
    wm_button button;
    wm_key key;
    uint32_t modifiers;
} wm_event;

const char *wm_status_string(wm_status status);

#endif
