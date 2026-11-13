#ifndef RETRO_MENU_H
#define RETRO_MENU_H

#include "retro_wm.h"

enum {
    RM_MAX_MENUS = 8,
    RM_MAX_ITEMS = 16,
    RM_BAR_HEIGHT = 28,
    RM_ITEM_HEIGHT = 24,
    RM_SEPARATOR_HEIGHT = 8
};

typedef enum {
    RM_ITEM_COMMAND,
    RM_ITEM_SEPARATOR
} rm_item_type;

typedef struct {
    rm_item_type type;
    const char *label;
    const char *shortcut;
    uint32_t command;
    bool enabled, checked;
} rm_item;

typedef struct {
    const char *label;
    const rm_item *items;
    size_t count;
} rm_menu;

typedef struct {
    wm_rect button, popup;
} rm_menu_bounds;

typedef struct {
    bool consumed;
    uint32_t command;
} rm_result;

/* Concrete storage permits static allocation; use the API to change state. */
typedef struct {
    const rm_menu *menus;
    size_t count;
    int32_t width, height;
    rm_menu_bounds bounds[RM_MAX_MENUS];
    wm_rect rows[RM_MAX_MENUS][RM_MAX_ITEMS];
    int32_t active_menu, active_item;
    bool dirty;
} rm_context;

wm_status rm_init(rm_context *context, int32_t width, int32_t height,
                   const rm_menu *menus, size_t count);
wm_status rm_handle_event(rm_context *context, const wm_event *event,
                          rm_result *result);
wm_status rm_render(rm_context *context, wm_framebuffer *framebuffer,
                     const wm_theme *theme);
wm_status rm_get_menu_bounds(const rm_context *context, size_t menu,
                             rm_menu_bounds *bounds);
wm_status rm_get_item_bounds(const rm_context *context, size_t menu,
                             size_t item, wm_rect *bounds);
bool rm_is_open(const rm_context *context);
bool rm_needs_repaint(const rm_context *context);
void rm_invalidate(rm_context *context);

#endif
