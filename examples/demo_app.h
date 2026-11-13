#ifndef DEMO_APP_H
#define DEMO_APP_H

#include "retro_ui.h"

enum { DEMO_WIDTH = 960, DEMO_HEIGHT = 640 };
enum { DEMO_MENU_FILE, DEMO_MENU_VIEW, DEMO_MENU_WINDOW, DEMO_MENU_HELP, DEMO_MENU_COUNT };

typedef struct {
    wm_color panel_background[3], bar_fill[3], text;
} demo_colours;

typedef struct {
    wm_id id;
    unsigned kind;
    const demo_colours *colours;
} demo_panel;

typedef struct {
    ui_context *ui;
    demo_colours colours;
    demo_panel panels[WM_MAX_WINDOWS];
    rm_item file_items[4], view_items[2], window_items[2], help_items[1];
    rm_menu menus[DEMO_MENU_COUNT];
    unsigned created;
    bool colour, running;
} demo_app;

wm_status demo_app_init(demo_app *app, ui_context *ui,
                        const wm_framebuffer *framebuffer);
wm_status demo_app_handle_event(demo_app *app, const wm_event *event);

#endif
