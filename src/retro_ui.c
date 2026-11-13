#include "retro_ui.h"

#include <assert.h>

wm_status ui_init(ui_context *context, const wm_framebuffer *framebuffer,
                   const rm_menu *menus, size_t count)
{
    if (context == NULL || framebuffer == NULL || framebuffer->pixels == NULL ||
        framebuffer->width < WM_MIN_WIDTH ||
        framebuffer->height < WM_MIN_HEIGHT + RM_BAR_HEIGHT ||
        framebuffer->stride_pixels < (size_t)framebuffer->width ||
        framebuffer->stride_pixels > SIZE_MAX / sizeof(uint32_t) /
                                     (size_t)framebuffer->height) {
        return WM_ERROR_ARGUMENT;
    }
    ui_context next = {
        .framebuffer = *framebuffer, .window_framebuffer = *framebuffer
    };
    next.window_framebuffer.pixels += (size_t)RM_BAR_HEIGHT * framebuffer->stride_pixels;
    next.window_framebuffer.height -= RM_BAR_HEIGHT;
    wm_status status = wm_init(&next.windows, framebuffer->width,
                               next.window_framebuffer.height);
    if (status != WM_OK) return status;
    status = rm_init(&next.menus, framebuffer->width, framebuffer->height, menus, count);
    if (status != WM_OK) return status;
    *context = next;
    return WM_OK;
}

wm_status ui_handle_event(ui_context *context, const wm_event *event,
                          ui_result *result)
{
    if (context == NULL || event == NULL || result == NULL) {
        return WM_ERROR_ARGUMENT;
    }
    *result = (ui_result){0};
    bool pointer = event->type == WM_EVENT_POINTER_MOVE ||
                   event->type == WM_EVENT_POINTER_DOWN ||
                   event->type == WM_EVENT_POINTER_UP;
    if (!pointer || !wm_has_capture(&context->windows)) {
        bool menu_was_open = rm_is_open(&context->menus);
        rm_result menu_result;
        wm_status status = rm_handle_event(&context->menus, event, &menu_result);
        if (status != WM_OK) return status;
        result->menu_consumed = menu_result.consumed;
        result->command = menu_result.command;
        if ((!menu_was_open && rm_is_open(&context->menus)) ||
            event->type == WM_EVENT_CANCEL) {
            wm_event cancel = {.type = WM_EVENT_CANCEL};
            status = wm_handle_event(&context->windows, &cancel, &result->window);
            if (status != WM_OK) return status;
        }
        if (result->menu_consumed || event->type == WM_EVENT_CANCEL) return WM_OK;
    }
    wm_event window_event = *event;
    if (pointer) {
        int64_t y = (int64_t)event->y - RM_BAR_HEIGHT;
        window_event.y = y < INT32_MIN ? INT32_MIN : (int32_t)y;
    }
    return wm_handle_event(&context->windows, &window_event, &result->window);
}

wm_status ui_render(ui_context *context)
{
    if (context == NULL) return WM_ERROR_ARGUMENT;
    wm_status status = wm_render(&context->windows, &context->window_framebuffer);
    if (status != WM_OK) return status;
    return rm_render(&context->menus, &context->framebuffer, &context->windows.theme);
}

bool ui_has_capture(const ui_context *context)
{
    assert(context != NULL);
    return wm_has_capture(&context->windows) || rm_is_open(&context->menus);
}

bool ui_needs_repaint(const ui_context *context)
{
    assert(context != NULL);
    return wm_needs_repaint(&context->windows) || rm_needs_repaint(&context->menus);
}

void ui_invalidate(ui_context *context)
{
    assert(context != NULL);
    wm_invalidate(&context->windows);
}
