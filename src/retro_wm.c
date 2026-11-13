#include "retro_wm.h"

#include <assert.h>
#include <string.h>

static const wm_window *find_window(const wm_context *context, wm_id id)
{
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        if (context->windows[i].id == id && id != WM_NO_WINDOW) {
            return &context->windows[i];
        }
    }
    return NULL;
}

static wm_window *find_mutable_window(wm_context *context, wm_id id)
{
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        if (context->windows[i].id == id && id != WM_NO_WINDOW) {
            return &context->windows[i];
        }
    }
    return NULL;
}

static bool contains(wm_rect rect, int32_t x, int32_t y)
{
    return x >= rect.x && y >= rect.y &&
           (int64_t)x < (int64_t)rect.x + rect.width &&
           (int64_t)y < (int64_t)rect.y + rect.height;
}

static wm_rect client_rect(wm_rect bounds)
{
    return (wm_rect){
        bounds.x + WM_BORDER,
        bounds.y + WM_BORDER + WM_TITLE_HEIGHT + 1,
        bounds.width - 2 * WM_BORDER,
        bounds.height - 2 * WM_BORDER - WM_TITLE_HEIGHT - 1
    };
}

static wm_rect close_rect(wm_rect bounds)
{
    return (wm_rect){
        bounds.x + bounds.width - WM_BORDER - 20,
        bounds.y + WM_BORDER + 4, 16, 16
    };
}

static wm_rect resize_rect(wm_rect bounds)
{
    return (wm_rect){
        bounds.x + bounds.width - WM_BORDER - WM_RESIZE_SIZE,
        bounds.y + bounds.height - WM_BORDER - WM_RESIZE_SIZE,
        WM_RESIZE_SIZE, WM_RESIZE_SIZE
    };
}

static int32_t clamp(int64_t value, int32_t low, int32_t high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return (int32_t)value;
}

static bool equal_rect(wm_rect a, wm_rect b)
{
    return a.x == b.x && a.y == b.y &&
           a.width == b.width && a.height == b.height;
}

static void cancel_capture(wm_context *context)
{
    if (context->interaction != WM_INTERACTION_NONE) {
        context->dirty = true;
    }
    context->interaction = WM_INTERACTION_NONE;
    context->captured = WM_NO_WINDOW;
    context->close_armed = false;
}

const char *wm_status_string(wm_status status)
{
    switch (status) {
    case WM_OK: return "success";
    case WM_ERROR_ARGUMENT: return "invalid argument";
    case WM_ERROR_NOT_FOUND: return "window not found";
    case WM_ERROR_CAPACITY: return "window or identifier capacity exhausted";
    }
    return "unknown window-manager status";
}

wm_theme wm_theme_monochrome(void)
{
    const wm_color black = WM_RGB(0, 0, 0);
    const wm_color white = WM_RGB(255, 255, 255);
    return (wm_theme){
        white, black, black, black, white, white, black,
        white, black, white
    };
}

wm_theme wm_theme_colour(void)
{
    return (wm_theme){
        WM_RGB(48, 91, 108), WM_RGB(54, 100, 117), WM_RGB(25, 33, 48),
        WM_RGB(42, 69, 132), WM_RGB(177, 190, 207),
        WM_RGB(255, 255, 255), WM_RGB(25, 33, 48),
        WM_RGB(245, 241, 224), WM_RGB(255, 215, 72), WM_RGB(25, 33, 48)
    };
}

wm_status wm_init(wm_context *context, int32_t width, int32_t height)
{
    if (context == NULL || width < WM_MIN_WIDTH || height < WM_MIN_HEIGHT) {
        return WM_ERROR_ARGUMENT;
    }
    memset(context, 0, sizeof(*context));
    context->width = width;
    context->height = height;
    context->next_id = 1;
    context->theme = wm_theme_colour();
    context->dirty = true;
    return WM_OK;
}

wm_status wm_set_theme(wm_context *context, const wm_theme *theme)
{
    if (context == NULL || theme == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    context->theme = *theme;
    context->dirty = true;
    return WM_OK;
}

wm_id wm_front_window(const wm_context *context)
{
    assert(context != NULL);
    return context->count == 0 ? WM_NO_WINDOW : context->order[context->count - 1];
}

size_t wm_window_count(const wm_context *context)
{
    assert(context != NULL);
    return context->count;
}

bool wm_has_capture(const wm_context *context)
{
    assert(context != NULL);
    return context->interaction != WM_INTERACTION_NONE;
}

bool wm_needs_repaint(const wm_context *context)
{
    assert(context != NULL);
    return context->dirty;
}

void wm_invalidate(wm_context *context)
{
    assert(context != NULL);
    context->dirty = true;
}

wm_status wm_create_window(wm_context *context, const wm_window_desc *desc,
                           wm_id *id)
{
    if (context == NULL || desc == NULL || id == NULL || desc->title == NULL ||
        desc->min_width < WM_MIN_WIDTH || desc->min_height < WM_MIN_HEIGHT ||
        desc->bounds.width < desc->min_width ||
        desc->bounds.height < desc->min_height ||
        desc->bounds.x < 0 || desc->bounds.y < 0 ||
        (int64_t)desc->bounds.x + desc->bounds.width > context->width ||
        (int64_t)desc->bounds.y + desc->bounds.height > context->height ||
        strlen(desc->title) >= WM_TITLE_CAPACITY) {
        return WM_ERROR_ARGUMENT;
    }
    if (context->count == WM_MAX_WINDOWS || context->next_id == WM_NO_WINDOW) {
        return WM_ERROR_CAPACITY;
    }
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        wm_window *window = &context->windows[i];
        if (window->id == WM_NO_WINDOW) {
            cancel_capture(context);
            window->id = context->next_id++;
            window->bounds = desc->bounds;
            window->min_width = desc->min_width;
            window->min_height = desc->min_height;
            memcpy(window->title, desc->title, strlen(desc->title) + 1);
            window->paint = desc->paint;
            window->userdata = desc->userdata;
            context->order[context->count++] = window->id;
            context->dirty = true;
            *id = window->id;
            return WM_OK;
        }
    }
    return WM_ERROR_CAPACITY;
}

wm_status wm_close_window(wm_context *context, wm_id id)
{
    if (context == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    wm_window *window = find_mutable_window(context, id);
    if (window == NULL) {
        return WM_ERROR_NOT_FOUND;
    }
    if (context->captured == id) {
        cancel_capture(context);
    }
    size_t index = 0;
    while (index < context->count && context->order[index] != id) {
        ++index;
    }
    assert(index < context->count);
    memmove(&context->order[index], &context->order[index + 1],
            (context->count - index - 1) * sizeof(context->order[0]));
    --context->count;
    memset(window, 0, sizeof(*window));
    context->dirty = true;
    return WM_OK;
}

wm_status wm_activate_window(wm_context *context, wm_id id)
{
    if (context == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    if (find_window(context, id) == NULL) {
        return WM_ERROR_NOT_FOUND;
    }
    if (context->captured != id) {
        cancel_capture(context);
    }
    if (wm_front_window(context) != id) {
        size_t index = 0;
        while (index < context->count && context->order[index] != id) {
            ++index;
        }
        assert(index < context->count);
        memmove(&context->order[index], &context->order[index + 1],
                (context->count - index - 1) * sizeof(context->order[0]));
        context->order[context->count - 1] = id;
        context->dirty = true;
    }
    return WM_OK;
}

wm_status wm_get_window_bounds(const wm_context *context, wm_id id,
                               wm_rect *bounds)
{
    if (context == NULL || bounds == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    const wm_window *window = find_window(context, id);
    if (window == NULL) {
        return WM_ERROR_NOT_FOUND;
    }
    *bounds = window->bounds;
    return WM_OK;
}

wm_status wm_hit_test(const wm_context *context, int32_t x, int32_t y,
                      wm_hit *hit)
{
    if (context == NULL || hit == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    *hit = (wm_hit){ WM_NO_WINDOW, WM_HIT_NONE };
    for (size_t i = context->count; i > 0; --i) {
        const wm_window *window = find_window(context, context->order[i - 1]);
        assert(window != NULL);
        wm_rect bounds = window->bounds;
        if (!contains(bounds, x, y)) {
            continue;
        }
        hit->window = window->id;
        if (contains(close_rect(bounds), x, y)) {
            hit->part = WM_HIT_CLOSE;
        } else if (contains(resize_rect(bounds), x, y)) {
            hit->part = WM_HIT_RESIZE;
        } else if (contains((wm_rect){
                bounds.x + WM_BORDER, bounds.y + WM_BORDER,
                bounds.width - 2 * WM_BORDER, WM_TITLE_HEIGHT }, x, y)) {
            hit->part = WM_HIT_TITLE;
        } else if (contains(client_rect(bounds), x, y)) {
            hit->part = WM_HIT_CLIENT;
        } else {
            hit->part = WM_HIT_BORDER;
        }
        break;
    }
    return WM_OK;
}

static void update_capture(wm_context *context, int32_t x, int32_t y)
{
    wm_window *window = find_mutable_window(context, context->captured);
    assert(window != NULL);
    int64_t dx = (int64_t)x - context->press_x;
    int64_t dy = (int64_t)y - context->press_y;
    wm_rect pending = context->original;
    if (context->interaction == WM_INTERACTION_DRAG) {
        pending.x = clamp((int64_t)pending.x + dx, 0,
                          context->width - pending.width);
        pending.y = clamp((int64_t)pending.y + dy, 0,
                          context->height - pending.height);
    } else if (context->interaction == WM_INTERACTION_RESIZE) {
        pending.width = clamp((int64_t)pending.width + dx, window->min_width,
                              context->width - pending.x);
        pending.height = clamp((int64_t)pending.height + dy, window->min_height,
                               context->height - pending.y);
    } else if (context->interaction == WM_INTERACTION_CLOSE) {
        bool armed = contains(close_rect(window->bounds), x, y);
        if (armed != context->close_armed) {
            context->close_armed = armed;
            context->dirty = true;
        }
    }
    if (!equal_rect(pending, context->pending)) {
        context->pending = pending;
        context->dirty = true;
    }
}

wm_status wm_handle_event(wm_context *context, const wm_event *event,
                          wm_update *update)
{
    if (context == NULL || event == NULL ||
        event->type < WM_EVENT_POINTER_MOVE || event->type > WM_EVENT_CANCEL ||
        ((event->type == WM_EVENT_POINTER_DOWN ||
          event->type == WM_EVENT_POINTER_UP) &&
         (event->button < WM_BUTTON_PRIMARY || event->button > WM_BUTTON_MIDDLE))) {
        return WM_ERROR_ARGUMENT;
    }
    wm_update result = { WM_NO_WINDOW, 0 };
    wm_id previous_front = wm_front_window(context);
    if (event->type == WM_EVENT_CANCEL ||
        (event->type == WM_EVENT_KEY_DOWN && event->key == WM_KEY_ESCAPE)) {
        result.window = context->captured;
        cancel_capture(context);
    } else if (event->type == WM_EVENT_KEY_DOWN) {
        if (event->key == WM_KEY_TAB && context->count > 1) {
            cancel_capture(context);
            size_t index = (event->modifiers & WM_MOD_SHIFT) != 0
                         ? context->count - 2 : 0;
            result.window = context->order[index];
            wm_status status = wm_activate_window(context, result.window);
            assert(status == WM_OK);
            (void)status;
        } else if (((event->key == WM_KEY_F4 &&
                     (event->modifiers & WM_MOD_ALT) != 0) ||
                    (event->key == WM_KEY_W &&
                     (event->modifiers & WM_MOD_CTRL) != 0)) &&
                   previous_front != WM_NO_WINDOW) {
            result.window = previous_front;
            wm_status status = wm_close_window(context, result.window);
            assert(status == WM_OK);
            (void)status;
            result.changes |= WM_UPDATE_CLOSED;
        }
    } else if (event->type == WM_EVENT_POINTER_DOWN &&
               event->button == WM_BUTTON_PRIMARY && !wm_has_capture(context)) {
        wm_hit hit;
        wm_status status = wm_hit_test(context, event->x, event->y, &hit);
        assert(status == WM_OK);
        (void)status;
        result.window = hit.window;
        if (hit.window != WM_NO_WINDOW) {
            status = wm_activate_window(context, hit.window);
            assert(status == WM_OK);
            const wm_window *window = find_window(context, hit.window);
            assert(window != NULL);
            if (hit.part == WM_HIT_TITLE || hit.part == WM_HIT_RESIZE ||
                hit.part == WM_HIT_CLOSE) {
                context->interaction = hit.part == WM_HIT_TITLE
                    ? WM_INTERACTION_DRAG : hit.part == WM_HIT_RESIZE
                    ? WM_INTERACTION_RESIZE : WM_INTERACTION_CLOSE;
                context->captured = hit.window;
                context->press_x = event->x;
                context->press_y = event->y;
                context->original = window->bounds;
                context->pending = window->bounds;
                context->close_armed = hit.part == WM_HIT_CLOSE;
                context->dirty = true;
            }
        }
    } else if (event->type == WM_EVENT_POINTER_MOVE && wm_has_capture(context)) {
        result.window = context->captured;
        update_capture(context, event->x, event->y);
    } else if (event->type == WM_EVENT_POINTER_UP &&
               event->button == WM_BUTTON_PRIMARY && wm_has_capture(context)) {
        result.window = context->captured;
        update_capture(context, event->x, event->y);
        wm_window *window = find_mutable_window(context, context->captured);
        assert(window != NULL);
        if (context->interaction == WM_INTERACTION_CLOSE && context->close_armed) {
            wm_status status = wm_close_window(context, result.window);
            assert(status == WM_OK);
            (void)status;
            result.changes |= WM_UPDATE_CLOSED;
        } else if (!equal_rect(context->original, context->pending)) {
            result.changes |= context->interaction == WM_INTERACTION_DRAG
                            ? WM_UPDATE_MOVED : WM_UPDATE_RESIZED;
            window->bounds = context->pending;
        }
        cancel_capture(context);
    }
    if (wm_front_window(context) != previous_front) {
        result.changes |= WM_UPDATE_FOCUS;
    }
    if (context->dirty) {
        result.changes |= WM_UPDATE_REPAINT;
    }
    if (update != NULL) {
        *update = result;
    }
    return WM_OK;
}

static void fill_absolute(wm_canvas *canvas, int64_t x, int64_t y,
                          int64_t width, int64_t height, wm_color color)
{
    if (width <= 0 || height <= 0) {
        return;
    }
    int64_t right = x + width;
    int64_t bottom = y + height;
    int64_t clip_right = (int64_t)canvas->clip.x + canvas->clip.width;
    int64_t clip_bottom = (int64_t)canvas->clip.y + canvas->clip.height;
    if (x < canvas->clip.x) x = canvas->clip.x;
    if (y < canvas->clip.y) y = canvas->clip.y;
    if (right > clip_right) right = clip_right;
    if (bottom > clip_bottom) bottom = clip_bottom;
    for (int64_t row = y; row < bottom; ++row) {
        uint32_t *pixels = canvas->framebuffer->pixels +
                           (size_t)row * canvas->framebuffer->stride_pixels;
        for (int64_t col = x; col < right; ++col) {
            pixels[col] = color;
        }
    }
}

void wm_canvas_fill(wm_canvas *canvas, wm_rect rect, wm_color color)
{
    assert(canvas != NULL && canvas->framebuffer != NULL);
    fill_absolute(canvas, (int64_t)canvas->origin_x + rect.x,
                  (int64_t)canvas->origin_y + rect.y,
                  rect.width, rect.height, color);
}

void wm_canvas_frame(wm_canvas *canvas, wm_rect rect, wm_color color)
{
    assert(canvas != NULL && canvas->framebuffer != NULL);
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    int64_t x = (int64_t)canvas->origin_x + rect.x;
    int64_t y = (int64_t)canvas->origin_y + rect.y;
    fill_absolute(canvas, x, y, rect.width, 1, color);
    fill_absolute(canvas, x, y + rect.height - 1, rect.width, 1, color);
    fill_absolute(canvas, x, y, 1, rect.height, color);
    fill_absolute(canvas, x + rect.width - 1, y, 1, rect.height, color);
}

typedef struct {
    char character;
    uint8_t rows[7];
} glyph;

static const glyph font[] = {
    { 'A', {14,17,17,31,17,17,17} }, { 'B', {30,17,17,30,17,17,30} },
    { 'C', {14,17,16,16,16,17,14} }, { 'D', {30,17,17,17,17,17,30} },
    { 'E', {31,16,16,30,16,16,31} }, { 'F', {31,16,16,30,16,16,16} },
    { 'G', {14,17,16,23,17,17,14} }, { 'H', {17,17,17,31,17,17,17} },
    { 'I', {14,4,4,4,4,4,14} }, { 'J', {7,2,2,2,2,18,12} },
    { 'K', {17,18,20,24,20,18,17} }, { 'L', {16,16,16,16,16,16,31} },
    { 'M', {17,27,21,21,17,17,17} }, { 'N', {17,25,25,21,19,19,17} },
    { 'O', {14,17,17,17,17,17,14} }, { 'P', {30,17,17,30,16,16,16} },
    { 'Q', {14,17,17,17,21,18,13} }, { 'R', {30,17,17,30,20,18,17} },
    { 'S', {15,16,16,14,1,1,30} }, { 'T', {31,4,4,4,4,4,4} },
    { 'U', {17,17,17,17,17,17,14} }, { 'V', {17,17,17,17,17,10,4} },
    { 'W', {17,17,17,21,21,27,17} }, { 'X', {17,17,10,4,10,17,17} },
    { 'Y', {17,17,10,4,4,4,4} }, { 'Z', {31,1,2,4,8,16,31} },
    { '0', {14,17,19,21,25,17,14} }, { '1', {4,12,4,4,4,4,14} },
    { '2', {14,17,1,2,4,8,31} }, { '3', {30,1,1,14,1,1,30} },
    { '4', {2,6,10,18,31,2,2} }, { '5', {31,16,16,30,1,1,30} },
    { '6', {14,16,16,30,17,17,14} }, { '7', {31,1,2,4,8,8,8} },
    { '8', {14,17,17,14,17,17,14} }, { '9', {14,17,17,15,1,1,14} },
    { '-', {0,0,0,31,0,0,0} }, { '.', {0,0,0,0,0,12,12} },
    { ':', {0,12,12,0,12,12,0} }, { '/', {1,2,2,4,8,8,16} },
    { '+', {0,4,4,31,4,4,0} }, { '=', {0,0,31,0,31,0,0} },
    { '(', {2,4,8,8,8,4,2} }, { ')', {8,4,2,2,2,4,8} },
    { '?', {14,17,1,2,4,0,4} }
};

static const uint8_t *glyph_rows(char character)
{
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    for (size_t i = 0; i < sizeof(font) / sizeof(font[0]); ++i) {
        if (font[i].character == character) {
            return font[i].rows;
        }
    }
    return font[sizeof(font) / sizeof(font[0]) - 1].rows;
}

void wm_canvas_text(wm_canvas *canvas, int32_t x, int32_t y,
                    const char *text, wm_color color)
{
    assert(canvas != NULL && canvas->framebuffer != NULL && text != NULL);
    int64_t cursor_x = (int64_t)canvas->origin_x + x;
    int64_t cursor_y = (int64_t)canvas->origin_y + y;
    int64_t start_x = cursor_x;
    for (; *text != '\0'; ++text) {
        if (*text == '\n') {
            cursor_x = start_x;
            cursor_y += WM_TEXT_HEIGHT + 4;
            continue;
        }
        if (*text != ' ') {
            const uint8_t *rows = glyph_rows(*text);
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if ((rows[row] & (1u << (4 - col))) != 0) {
                        fill_absolute(canvas, cursor_x + 2 * col,
                                      cursor_y + 2 * row, 2, 2, color);
                    }
                }
            }
        }
        cursor_x += WM_TEXT_WIDTH;
    }
}

static void draw_window(wm_context *context, wm_framebuffer *framebuffer,
                        const wm_window *window)
{
    wm_canvas screen = {
        framebuffer, {0, 0, framebuffer->width, framebuffer->height}, 0, 0
    };
    wm_rect bounds = window->bounds;
    wm_rect title = {
        bounds.x + WM_BORDER, bounds.y + WM_BORDER,
        bounds.width - 2 * WM_BORDER, WM_TITLE_HEIGHT
    };
    bool active = window->id == wm_front_window(context);
    wm_color title_color = active ? context->theme.active_title
                                 : context->theme.inactive_title;
    wm_color text_color = active ? context->theme.active_text
                                : context->theme.inactive_text;
    wm_canvas_fill(&screen, bounds, context->theme.border);
    wm_canvas_fill(&screen, title, title_color);
    wm_canvas title_canvas = screen;
    title_canvas.clip = (wm_rect){title.x + 5, title.y, title.width - 29, title.height};
    wm_canvas_text(&title_canvas, title.x + 5, title.y + 5,
                   window->title, text_color);

    wm_rect client = client_rect(bounds);
    wm_canvas_fill(&screen, client, context->theme.client);
    if (window->paint != NULL) {
        wm_canvas canvas = {framebuffer, client, client.x, client.y};
        window->paint(&canvas, client.width, client.height, window->userdata);
    }

    wm_rect close = close_rect(bounds);
    bool pressed = context->captured == window->id &&
                   context->interaction == WM_INTERACTION_CLOSE &&
                   context->close_armed;
    wm_color close_fill = pressed ? context->theme.border : context->theme.client;
    wm_color close_ink = pressed ? context->theme.client : context->theme.border;
    wm_canvas_fill(&screen, close, close_fill);
    wm_canvas_frame(&screen, close, close_ink);
    for (int i = 4; i < 12; ++i) {
        wm_canvas_fill(&screen, (wm_rect){close.x + i, close.y + i, 1, 1}, close_ink);
        wm_canvas_fill(&screen, (wm_rect){close.x + 15 - i, close.y + i, 1, 1}, close_ink);
    }
    wm_rect grip = resize_rect(bounds);
    wm_canvas_fill(&screen, grip, context->theme.client);
    for (int line = 3; line < WM_RESIZE_SIZE; line += 4) {
        for (int i = 0; i < line; ++i) {
            wm_canvas_fill(&screen, (wm_rect){
                grip.x + WM_RESIZE_SIZE - line + i,
                grip.y + WM_RESIZE_SIZE - 1 - i, 1, 1
            }, context->theme.border);
        }
    }
}

static void draw_outline(wm_context *context, wm_framebuffer *framebuffer)
{
    wm_canvas canvas = {
        framebuffer, {0, 0, framebuffer->width, framebuffer->height}, 0, 0
    };
    wm_rect rect = context->pending;
    for (int32_t x = rect.x; x < rect.x + rect.width; ++x) {
        wm_color color = ((x - rect.x) / 4) % 2 == 0
                       ? context->theme.outline : context->theme.outline_alt;
        wm_canvas_fill(&canvas, (wm_rect){x, rect.y, 1, 2}, color);
        wm_canvas_fill(&canvas, (wm_rect){x, rect.y + rect.height - 2, 1, 2}, color);
    }
    for (int32_t y = rect.y; y < rect.y + rect.height; ++y) {
        wm_color color = ((y - rect.y) / 4) % 2 == 0
                       ? context->theme.outline : context->theme.outline_alt;
        wm_canvas_fill(&canvas, (wm_rect){rect.x, y, 2, 1}, color);
        wm_canvas_fill(&canvas, (wm_rect){rect.x + rect.width - 2, y, 2, 1}, color);
    }
}

wm_status wm_render(wm_context *context, wm_framebuffer *framebuffer)
{
    if (context == NULL || framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width != context->width ||
        framebuffer->height != context->height ||
        framebuffer->stride_pixels < (size_t)context->width ||
        framebuffer->stride_pixels > SIZE_MAX / sizeof(uint32_t) /
                                     (size_t)context->height) {
        return WM_ERROR_ARGUMENT;
    }
    for (int32_t y = 0; y < context->height; ++y) {
        uint32_t *row = framebuffer->pixels + (size_t)y * framebuffer->stride_pixels;
        for (int32_t x = 0; x < context->width; ++x) {
            row[x] = ((x ^ y) & 1) == 0
                   ? context->theme.desktop : context->theme.desktop_alt;
        }
    }
    for (size_t i = 0; i < context->count; ++i) {
        const wm_window *window = find_window(context, context->order[i]);
        assert(window != NULL);
        draw_window(context, framebuffer, window);
    }
    if (context->interaction == WM_INTERACTION_DRAG ||
        context->interaction == WM_INTERACTION_RESIZE) {
        draw_outline(context, framebuffer);
    }
    context->dirty = false;
    return WM_OK;
}
