#ifndef RETRO_UI_H
#define RETRO_UI_H

#include "retro_menu.h"
#include "retro_wm.h"

typedef struct {
    bool menu_consumed;
    uint32_t command;
    wm_update window;
} ui_result;

/* Use wm_/rm_ APIs on the public components; ui_ owns their composition. */
typedef struct {
    wm_context windows;
    rm_context menus;
    wm_framebuffer framebuffer, window_framebuffer;
} ui_context;

wm_status ui_init(ui_context *context, const wm_framebuffer *framebuffer,
                   const rm_menu *menus, size_t count);
wm_status ui_handle_event(ui_context *context, const wm_event *event,
                          ui_result *result);
wm_status ui_render(ui_context *context);
bool ui_has_capture(const ui_context *context);
bool ui_needs_repaint(const ui_context *context);
void ui_invalidate(ui_context *context);

#endif
