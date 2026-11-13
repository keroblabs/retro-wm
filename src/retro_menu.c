#include "retro_menu.h"

#include <assert.h>
#include <string.h>

enum { PADDING = 5, CHECK_WIDTH = 20, SHORTCUT_GAP = 24, LABEL_LIMIT = 64 };

static bool contains(wm_rect rect, int32_t x, int32_t y)
{
    return x >= rect.x && y >= rect.y &&
           (int64_t)x < (int64_t)rect.x + rect.width &&
           (int64_t)y < (int64_t)rect.y + rect.height;
}

static bool selectable(const rm_item *item)
{
    return item->type == RM_ITEM_COMMAND && item->enabled;
}

wm_status rm_init(rm_context *context, int32_t width, int32_t height,
                   const rm_menu *menus, size_t count)
{
    if (context == NULL || menus == NULL || count == 0 || count > RM_MAX_MENUS ||
        width < WM_MIN_WIDTH || height < RM_BAR_HEIGHT + RM_ITEM_HEIGHT + 12) {
        return WM_ERROR_ARGUMENT;
    }
    rm_context next = {
        .menus = menus, .count = count, .width = width, .height = height,
        .active_menu = -1, .active_item = -1, .dirty = true
    };
    int32_t button_x = 8;
    for (size_t m = 0; m < count; ++m) {
        const rm_menu *menu = &menus[m];
        if (menu->label == NULL || menu->items == NULL ||
            menu->count == 0 || menu->count > RM_MAX_ITEMS) {
            return WM_ERROR_ARGUMENT;
        }
        size_t label_length = strlen(menu->label);
        if (label_length == 0 || label_length > LABEL_LIMIT) {
            return WM_ERROR_ARGUMENT;
        }
        int32_t button_width = (int32_t)label_length * WM_TEXT_WIDTH + 20;
        if ((int64_t)button_x + button_width > width) {
            return WM_ERROR_ARGUMENT;
        }
        int32_t popup_width = button_width;
        int32_t popup_height = 2 * PADDING + 2;
        for (size_t i = 0; i < menu->count; ++i) {
            const rm_item *item = &menu->items[i];
            if (item->type == RM_ITEM_SEPARATOR) {
                popup_height += RM_SEPARATOR_HEIGHT;
                continue;
            }
            if (item->type != RM_ITEM_COMMAND || item->label == NULL ||
                item->command == 0) {
                return WM_ERROR_ARGUMENT;
            }
            size_t length = strlen(item->label);
            size_t shortcut_length = item->shortcut == NULL ? 0 : strlen(item->shortcut);
            if (length == 0 || length > LABEL_LIMIT || shortcut_length > LABEL_LIMIT) {
                return WM_ERROR_ARGUMENT;
            }
            int32_t item_width = (int32_t)(length + shortcut_length) * WM_TEXT_WIDTH +
                                 CHECK_WIDTH + 2 * PADDING + 2;
            if (shortcut_length != 0) {
                item_width += SHORTCUT_GAP;
            }
            if (item_width > popup_width) popup_width = item_width;
            popup_height += RM_ITEM_HEIGHT;
        }
        if (popup_height > height - RM_BAR_HEIGHT) {
            return WM_ERROR_ARGUMENT;
        }
        if (popup_width > width) popup_width = width;
        int32_t popup_x = button_x;
        if (popup_x > width - popup_width) popup_x = width - popup_width;
        next.bounds[m] = (rm_menu_bounds){
            {button_x, 1, button_width, RM_BAR_HEIGHT - 3},
            {popup_x, RM_BAR_HEIGHT, popup_width, popup_height}
        };
        int32_t row_y = RM_BAR_HEIGHT + PADDING + 1;
        for (size_t i = 0; i < menu->count; ++i) {
            int32_t row_height = menu->items[i].type == RM_ITEM_SEPARATOR
                               ? RM_SEPARATOR_HEIGHT : RM_ITEM_HEIGHT;
            next.rows[m][i] = (wm_rect){popup_x + 1, row_y, popup_width - 2, row_height};
            row_y += row_height;
        }
        button_x += button_width;
    }
    *context = next;
    return WM_OK;
}

bool rm_is_open(const rm_context *context)
{
    assert(context != NULL);
    return context->active_menu >= 0;
}

bool rm_needs_repaint(const rm_context *context)
{
    assert(context != NULL);
    return context->dirty;
}

void rm_invalidate(rm_context *context)
{
    assert(context != NULL);
    context->dirty = true;
}

wm_status rm_get_menu_bounds(const rm_context *context, size_t menu,
                             rm_menu_bounds *bounds)
{
    if (context == NULL || bounds == NULL || menu >= context->count) {
        return WM_ERROR_ARGUMENT;
    }
    *bounds = context->bounds[menu];
    return WM_OK;
}

wm_status rm_get_item_bounds(const rm_context *context, size_t menu,
                             size_t item, wm_rect *bounds)
{
    if (context == NULL || bounds == NULL || menu >= context->count ||
        item >= context->menus[menu].count) {
        return WM_ERROR_ARGUMENT;
    }
    *bounds = context->rows[menu][item];
    return WM_OK;
}

static void highlight(rm_context *context, int32_t item)
{
    if (context->active_item != item) {
        context->active_item = item;
        context->dirty = true;
    }
}

static void close_menu(rm_context *context)
{
    context->active_menu = -1;
    context->active_item = -1;
    context->dirty = true;
}

static void step_item(rm_context *context, int32_t direction)
{
    const rm_menu *menu = &context->menus[context->active_menu];
    int32_t count = (int32_t)menu->count;
    int32_t item = context->active_item;
    if (item < 0) item = direction > 0 ? count - 1 : 0;
    for (int32_t i = 0; i < count; ++i) {
        item = (item + direction + count) % count;
        if (selectable(&menu->items[item])) {
            highlight(context, item);
            return;
        }
    }
    highlight(context, -1);
}

static void open_menu(rm_context *context, int32_t menu, bool keyboard)
{
    context->active_menu = menu;
    context->active_item = -1;
    context->dirty = true;
    if (keyboard) step_item(context, 1);
}

static int32_t button_at(const rm_context *context, int32_t x, int32_t y)
{
    for (size_t i = 0; i < context->count; ++i) {
        if (contains(context->bounds[i].button, x, y)) {
            return (int32_t)i;
        }
    }
    return -1;
}

static int32_t item_at(const rm_context *context, int32_t x, int32_t y)
{
    const rm_menu *menu = &context->menus[context->active_menu];
    for (size_t i = 0; i < menu->count; ++i) {
        if (selectable(&menu->items[i]) &&
            contains(context->rows[context->active_menu][i], x, y)) {
            return (int32_t)i;
        }
    }
    return -1;
}

static uint32_t activate_item(rm_context *context)
{
    if (context->active_item < 0) return 0;
    const rm_item *item = &context->menus[context->active_menu].items[context->active_item];
    if (!selectable(item)) return 0;
    uint32_t command = item->command;
    close_menu(context);
    return command;
}

wm_status rm_handle_event(rm_context *context, const wm_event *event,
                          rm_result *result)
{
    if (context == NULL || event == NULL || result == NULL ||
        event->type < WM_EVENT_POINTER_MOVE || event->type > WM_EVENT_CANCEL ||
        ((event->type == WM_EVENT_POINTER_DOWN || event->type == WM_EVENT_POINTER_UP) &&
         (event->button < WM_BUTTON_PRIMARY || event->button > WM_BUTTON_MIDDLE))) {
        return WM_ERROR_ARGUMENT;
    }
    bool pointer = event->type == WM_EVENT_POINTER_MOVE ||
                   event->type == WM_EVENT_POINTER_DOWN ||
                   event->type == WM_EVENT_POINTER_UP;
    bool on_bar = pointer && contains((wm_rect){0, 0, context->width, RM_BAR_HEIGHT},
                                       event->x, event->y);
    *result = (rm_result){rm_is_open(context) || on_bar, 0};
    int32_t button = pointer ? button_at(context, event->x, event->y) : -1;

    if (event->type == WM_EVENT_KEY_DOWN && event->key == WM_KEY_F10) {
        result->consumed = true;
        if (rm_is_open(context)) close_menu(context);
        else open_menu(context, 0, true);
    } else if (event->type == WM_EVENT_CANCEL) {
        if (rm_is_open(context)) close_menu(context);
    } else if (!rm_is_open(context)) {
        if (event->type == WM_EVENT_POINTER_DOWN &&
            event->button == WM_BUTTON_PRIMARY && button >= 0) {
            open_menu(context, button, false);
        }
    } else if (event->type == WM_EVENT_KEY_DOWN) {
        switch (event->key) {
        case WM_KEY_ESCAPE:
            close_menu(context);
            break;
        case WM_KEY_LEFT:
        case WM_KEY_RIGHT: {
            int32_t direction = event->key == WM_KEY_RIGHT ? 1 : -1;
            int32_t count = (int32_t)context->count;
            open_menu(context, (context->active_menu + direction + count) % count, true);
            break;
        }
        case WM_KEY_UP: step_item(context, -1); break;
        case WM_KEY_DOWN: step_item(context, 1); break;
        case WM_KEY_ENTER:
        case WM_KEY_SPACE: result->command = activate_item(context); break;
        default: break;
        }
    } else if (event->type == WM_EVENT_POINTER_MOVE) {
        if (button >= 0 && button != context->active_menu) {
            open_menu(context, button, false);
        }
        highlight(context, button >= 0 ? -1 : item_at(context, event->x, event->y));
    } else if (event->type == WM_EVENT_POINTER_DOWN &&
               event->button == WM_BUTTON_PRIMARY) {
        if (button == context->active_menu) {
            close_menu(context);
        } else if (button >= 0) {
            open_menu(context, button, false);
        } else if (!contains(context->bounds[context->active_menu].popup,
                             event->x, event->y)) {
            close_menu(context);
        } else {
            highlight(context, item_at(context, event->x, event->y));
        }
    } else if (event->type == WM_EVENT_POINTER_UP &&
               event->button == WM_BUTTON_PRIMARY) {
        if (button >= 0) {
            if (button != context->active_menu) open_menu(context, button, false);
        } else {
            highlight(context, item_at(context, event->x, event->y));
            result->command = activate_item(context);
            if (rm_is_open(context) &&
                !contains(context->bounds[context->active_menu].popup, event->x, event->y)) {
                close_menu(context);
            }
        }
    }
    return WM_OK;
}

static void draw_text(wm_canvas *canvas, wm_rect clip, int32_t x, int32_t y,
                      const char *text, bool enabled, wm_color foreground,
                      wm_color background)
{
    wm_canvas text_canvas = *canvas;
    text_canvas.clip = clip;
    wm_canvas_text(&text_canvas, x, y, text, foreground);
    if (!enabled) {
        for (int32_t row = clip.y; row < clip.y + clip.height; ++row) {
            for (int32_t col = clip.x; col < clip.x + clip.width; ++col) {
                if (((row ^ col) & 1) != 0) {
                    wm_canvas_fill(&text_canvas, (wm_rect){col, row, 1, 1}, background);
                }
            }
        }
    }
}

wm_status rm_render(rm_context *context, wm_framebuffer *framebuffer,
                     const wm_theme *theme)
{
    if (context == NULL || framebuffer == NULL || theme == NULL ||
        framebuffer->pixels == NULL || framebuffer->width != context->width ||
        framebuffer->height != context->height ||
        framebuffer->stride_pixels < (size_t)context->width ||
        framebuffer->stride_pixels > SIZE_MAX / sizeof(uint32_t) / (size_t)context->height) {
        return WM_ERROR_ARGUMENT;
    }
    wm_canvas canvas = {
        framebuffer, {0, 0, context->width, context->height}, 0, 0
    };
    wm_canvas_fill(&canvas, (wm_rect){0, 0, context->width, RM_BAR_HEIGHT}, theme->client);
    wm_canvas_fill(&canvas, (wm_rect){0, RM_BAR_HEIGHT - 2, context->width, 2}, theme->border);
    for (size_t m = 0; m < context->count; ++m) {
        wm_rect button = context->bounds[m].button;
        bool selected = (int32_t)m == context->active_menu;
        if (selected) wm_canvas_fill(&canvas, button, theme->active_title);
        wm_canvas_text(&canvas, button.x + 10, (RM_BAR_HEIGHT - WM_TEXT_HEIGHT) / 2,
                       context->menus[m].label,
                       selected ? theme->active_text : theme->border);
    }
    if (rm_is_open(context)) {
        int32_t active = context->active_menu;
        const rm_menu *menu = &context->menus[active];
        wm_rect popup = context->bounds[active].popup;
        wm_canvas_fill(&canvas, (wm_rect){
            popup.x + 3, popup.y + 3, popup.width, popup.height
        }, theme->border);
        wm_canvas_fill(&canvas, popup, theme->client);
        wm_canvas_frame(&canvas, popup, theme->border);
        for (size_t i = 0; i < menu->count; ++i) {
            const rm_item *item = &menu->items[i];
            wm_rect row = context->rows[active][i];
            if (item->type == RM_ITEM_SEPARATOR) {
                wm_canvas_fill(&canvas, (wm_rect){
                    row.x + PADDING, row.y + row.height / 2,
                    row.width - 2 * PADDING, 1
                }, theme->border);
                continue;
            }
            bool selected = (int32_t)i == context->active_item && item->enabled;
            wm_color background = selected ? theme->active_title : theme->client;
            wm_color foreground = selected ? theme->active_text : theme->border;
            wm_canvas_fill(&canvas, row, background);
            int32_t text_y = row.y + (row.height - WM_TEXT_HEIGHT) / 2;
            int32_t text_x = row.x + PADDING + CHECK_WIDTH;
            int32_t right = row.x + row.width - PADDING;
            int32_t shortcut_width = item->shortcut == NULL
                                   ? 0 : (int32_t)strlen(item->shortcut) * WM_TEXT_WIDTH;
            int32_t label_right = shortcut_width == 0
                                ? right : right - shortcut_width - SHORTCUT_GAP;
            int32_t label_width = label_right > text_x ? label_right - text_x : 0;
            draw_text(&canvas, (wm_rect){text_x, text_y, label_width, WM_TEXT_HEIGHT},
                      text_x, text_y, item->label, item->enabled, foreground, background);
            if (shortcut_width != 0) {
                draw_text(&canvas, (wm_rect){
                    text_x, text_y, right - text_x, WM_TEXT_HEIGHT
                }, right - shortcut_width, text_y, item->shortcut, item->enabled,
                foreground, background);
            }
            if (item->checked) {
                for (int32_t n = 0; n < 3; ++n) {
                    wm_canvas_fill(&canvas, (wm_rect){
                        row.x + 6 + n, text_y + 5 + n, 2, 2
                    }, foreground);
                }
                for (int32_t n = 0; n < 6; ++n) {
                    wm_canvas_fill(&canvas, (wm_rect){
                        row.x + 8 + n, text_y + 7 - n, 2, 2
                    }, foreground);
                }
            }
        }
    }
    context->dirty = false;
    return WM_OK;
}
