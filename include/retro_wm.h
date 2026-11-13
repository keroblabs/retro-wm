#ifndef RETRO_WM_H
#define RETRO_WM_H

#include "retro_types.h"
#include "retro_theme.h"

enum {
    WM_MAX_WINDOWS = 8,
    WM_TITLE_CAPACITY = 48,
    WM_BORDER = 2,
    WM_TITLE_HEIGHT = 24,
    WM_RESIZE_SIZE = 14,
    WM_MIN_WIDTH = 96,
    WM_MIN_HEIGHT = 64,
    WM_TEXT_WIDTH = 12,
    WM_TEXT_HEIGHT = 14
};

typedef uint32_t wm_id;

#define WM_NO_WINDOW UINT32_C(0)

typedef void (*wm_paint_fn)(wm_canvas *canvas, int32_t width,
                            int32_t height, void *userdata);

typedef struct {
    const char *title;
    wm_rect bounds;
    int32_t min_width, min_height;
    wm_paint_fn paint;
    void *userdata;
} wm_window_desc;

typedef enum {
    WM_HIT_NONE,
    WM_HIT_CLIENT,
    WM_HIT_TITLE,
    WM_HIT_CLOSE,
    WM_HIT_RESIZE,
    WM_HIT_BORDER
} wm_hit_part;

typedef struct {
    wm_id window;
    wm_hit_part part;
} wm_hit;

enum {
    WM_UPDATE_FOCUS = 1u << 0,
    WM_UPDATE_MOVED = 1u << 1,
    WM_UPDATE_RESIZED = 1u << 2,
    WM_UPDATE_CLOSED = 1u << 3,
    WM_UPDATE_REPAINT = 1u << 4
};

typedef struct {
    wm_id window;
    uint32_t changes;
} wm_update;

typedef enum {
    WM_INTERACTION_NONE,
    WM_INTERACTION_DRAG,
    WM_INTERACTION_RESIZE,
    WM_INTERACTION_CLOSE
} wm_interaction;

typedef struct {
    wm_id id;
    wm_rect bounds;
    int32_t min_width, min_height;
    char title[WM_TITLE_CAPACITY];
    wm_paint_fn paint;
    void *userdata;
} wm_window;

/* Exposed for static allocation; applications should use the API below. */
typedef struct {
    int32_t width, height;
    wm_theme theme;
    wm_window windows[WM_MAX_WINDOWS];
    wm_id order[WM_MAX_WINDOWS];
    size_t count;
    wm_id next_id;
    bool dirty;
    wm_interaction interaction;
    wm_id captured;
    int32_t press_x, press_y;
    wm_rect original, pending;
    bool close_armed;
} wm_context;

wm_status wm_init(wm_context *context, int32_t width, int32_t height);
wm_status wm_set_theme(wm_context *context, const wm_theme *theme);
wm_status wm_create_window(wm_context *context, const wm_window_desc *desc,
                           wm_id *id);
wm_status wm_close_window(wm_context *context, wm_id id);
wm_status wm_activate_window(wm_context *context, wm_id id);
wm_status wm_get_window_bounds(const wm_context *context, wm_id id,
                               wm_rect *bounds);
wm_status wm_hit_test(const wm_context *context, int32_t x, int32_t y,
                      wm_hit *hit);
wm_status wm_handle_event(wm_context *context, const wm_event *event,
                          wm_update *update);
wm_id wm_front_window(const wm_context *context);
size_t wm_window_count(const wm_context *context);
bool wm_has_capture(const wm_context *context);
bool wm_needs_repaint(const wm_context *context);
void wm_invalidate(wm_context *context);
wm_status wm_render(wm_context *context, wm_framebuffer *framebuffer);

void wm_canvas_fill(wm_canvas *canvas, wm_rect rect, wm_color color);
void wm_canvas_frame(wm_canvas *canvas, wm_rect rect, wm_color color);
void wm_canvas_text(wm_canvas *canvas, int32_t x, int32_t y,
                    const char *text, wm_color color);

#endif
