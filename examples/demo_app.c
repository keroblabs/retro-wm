#include "demo_app.h"

#include <stdio.h>

enum {
    ACTION_NEW = 1, ACTION_CLOSE, ACTION_QUIT, ACTION_COLOUR, ACTION_MONOCHROME,
    ACTION_NEXT, ACTION_PREVIOUS, ACTION_ABOUT
};

static const wm_rect initial_bounds[] = {
    {60, 64, 400, 280}, {260, 205, 380, 260}, {570, 80, 330, 218}
};

static void paint_panel(wm_canvas *canvas, int32_t width,
                        int32_t height, void *userdata)
{
    const demo_panel *panel = userdata;
    bool colour = *panel->colour;
    const wm_color backgrounds[] = {
        WM_RGB(238, 241, 252), WM_RGB(250, 239, 211), WM_RGB(226, 242, 231)
    };
    wm_color background = colour ? backgrounds[panel->kind % 3]
                                 : WM_RGB(255, 255, 255);
    wm_color ink = colour ? WM_RGB(28, 43, 62) : WM_RGB(0, 0, 0);
    wm_canvas_fill(canvas, (wm_rect){0, 0, width, height}, background);
    if (panel->kind == 0) {
        wm_canvas_text(canvas, 14, 14, "SOFTWARE FRAMEBUFFER", ink);
        wm_canvas_text(canvas, 14, 42, "NO SDL IN THE CORE", ink);
        wm_canvas_text(canvas, 14, 70, "DRAG THE TITLE BAR", ink);
        wm_canvas_text(canvas, 14, 98, "RESIZE AT THE CORNER", ink);
        const wm_color colors[] = {
            WM_RGB(68, 109, 188), WM_RGB(191, 82, 77), WM_RGB(63, 139, 102)
        };
        for (int i = 0; i < 3; ++i) {
            wm_rect bar = {14, 140 + i * 27, width - 28, 18};
            wm_canvas_frame(canvas, bar, ink);
            wm_canvas_fill(canvas, (wm_rect){
                bar.x + 3, bar.y + 3, (bar.width - 6) * (i + 2) / 5,
                bar.height - 6
            }, colour ? colors[i] : ink);
        }
    } else if (panel->kind == 1) {
        wm_canvas_text(canvas, 14, 14,
                       "POINTER CAPTURE\n\nRELEASE TO COMMIT\nESC TO CANCEL\n\n"
                       "TAB / SHIFT+TAB\nCTRL+W CLOSES", ink);
    } else if (panel->kind == 2) {
        wm_canvas_text(canvas, 14, 14,
                       "MENU BAR / F10\n\nN - NEW WINDOW\nC - COLOUR THEME\n"
                       "Q - QUIT HOST\n\nRISC-V BACKEND NEXT", ink);
    } else {
        wm_canvas_text(canvas, 14, 14,
                       "ANOTHER CLIENT\n\nCONTENT IS CLIPPED\nTO THIS WINDOW.\n\n"
                       "TRY OVERLAPPING\nAND CLOSING WINDOWS.", ink);
    }
}

static wm_status create_panel(demo_app *app, wm_rect bounds, const char *title,
                              unsigned kind, wm_id *id)
{
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        demo_panel *panel = &app->panels[i];
        if (panel->id == WM_NO_WINDOW) {
            panel->kind = kind;
            panel->colour = &app->colour;
            wm_window_desc desc = {title, bounds, 160, 100, paint_panel, panel};
            wm_status status = wm_create_window(&app->ui->windows, &desc, id);
            if (status == WM_OK) {
                panel->id = *id;
                ++app->created;
            }
            return status;
        }
    }
    return WM_ERROR_CAPACITY;
}

static void apply_window_update(demo_app *app, const wm_update *update)
{
    if ((update->changes & WM_UPDATE_CLOSED) != 0) {
        for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
            if (app->panels[i].id == update->window) {
                app->panels[i].id = WM_NO_WINDOW;
                break;
            }
        }
    }
}

static wm_status perform_action(demo_app *app, uint32_t command)
{
    switch (command) {
    case ACTION_NEW: {
        unsigned offset = (app->created % 5) * 24;
        char title[WM_TITLE_CAPACITY];
        int length = snprintf(title, sizeof(title), "WINDOW %u", app->created + 1);
        if (length < 0 || (size_t)length >= sizeof(title)) {
            fprintf(stderr, "Unable to format the window title\n");
            return WM_ERROR_ARGUMENT;
        }
        wm_id id;
        wm_status status = create_panel(app, (wm_rect){
            100 + (int32_t)offset, 100 + (int32_t)offset, 360, 240
        }, title, 3, &id);
        if (status == WM_ERROR_CAPACITY) {
            fprintf(stderr, "Window limit reached; close a window before creating another\n");
            return WM_OK;
        }
        return status;
    }
    case ACTION_CLOSE:
    case ACTION_NEXT:
    case ACTION_PREVIOUS: {
        wm_event event = {
            .type = WM_EVENT_KEY_DOWN,
            .key = command == ACTION_CLOSE ? WM_KEY_W : WM_KEY_TAB,
            .modifiers = command == ACTION_CLOSE ? WM_MOD_CTRL
                       : command == ACTION_PREVIOUS ? WM_MOD_SHIFT : 0
        };
        wm_update update;
        wm_status status = wm_handle_event(&app->ui->windows, &event, &update);
        if (status == WM_OK) apply_window_update(app, &update);
        return status;
    }
    case ACTION_COLOUR:
    case ACTION_MONOCHROME: {
        bool colour = command == ACTION_COLOUR;
        wm_theme theme = colour ? wm_theme_colour() : wm_theme_monochrome();
        wm_status status = wm_set_theme(&app->ui->windows, &theme);
        if (status == WM_OK) app->colour = colour;
        return status;
    }
    case ACTION_ABOUT:
        for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
            if (app->panels[i].id != WM_NO_WINDOW && app->panels[i].kind == 2) {
                return wm_activate_window(&app->ui->windows, app->panels[i].id);
            }
        }
        {
            wm_id id;
            return create_panel(app, initial_bounds[2], "PORTABLE C", 2, &id);
        }
    case ACTION_QUIT:
        app->running = false;
        return WM_OK;
    default:
        fprintf(stderr, "Unknown demo command: %u\n", (unsigned)command);
        return WM_ERROR_ARGUMENT;
    }
}

static void update_item(demo_app *app, rm_item *item, bool enabled, bool checked)
{
    if (item->enabled != enabled || item->checked != checked) {
        item->enabled = enabled;
        item->checked = checked;
        rm_invalidate(&app->ui->menus);
    }
}

static void refresh_menu_state(demo_app *app)
{
    size_t count = wm_window_count(&app->ui->windows);
    bool help_open = false;
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        if (app->panels[i].id != WM_NO_WINDOW && app->panels[i].kind == 2) {
            help_open = true;
        }
    }
    update_item(app, &app->file_items[0], count < WM_MAX_WINDOWS, false);
    update_item(app, &app->file_items[1], count != 0, false);
    update_item(app, &app->view_items[0], true, app->colour);
    update_item(app, &app->view_items[1], true, !app->colour);
    update_item(app, &app->window_items[0], count > 1, false);
    update_item(app, &app->window_items[1], count > 1, false);
    update_item(app, &app->help_items[0], help_open || count < WM_MAX_WINDOWS, false);
}

static void init_menu_descriptors(demo_app *app)
{
    app->file_items[0] = (rm_item){
        RM_ITEM_COMMAND, "New Window", "N", ACTION_NEW, true, false
    };
    app->file_items[1] = (rm_item){
        RM_ITEM_COMMAND, "Close Window", "CTRL+W", ACTION_CLOSE, true, false
    };
    app->file_items[2] = (rm_item){.type = RM_ITEM_SEPARATOR};
    app->file_items[3] = (rm_item){
        RM_ITEM_COMMAND, "Quit", "Q", ACTION_QUIT, true, false
    };
    app->view_items[0] = (rm_item){
        RM_ITEM_COMMAND, "Colour", NULL, ACTION_COLOUR, true, true
    };
    app->view_items[1] = (rm_item){
        RM_ITEM_COMMAND, "Monochrome", NULL, ACTION_MONOCHROME, true, false
    };
    app->window_items[0] = (rm_item){
        RM_ITEM_COMMAND, "Next Window", "TAB", ACTION_NEXT, true, false
    };
    app->window_items[1] = (rm_item){
        RM_ITEM_COMMAND, "Previous Window", "SHIFT+TAB", ACTION_PREVIOUS, true, false
    };
    app->help_items[0] = (rm_item){
        RM_ITEM_COMMAND, "About Retro WM", NULL, ACTION_ABOUT, true, false
    };
    app->menus[DEMO_MENU_FILE] = (rm_menu){"File", app->file_items, 4};
    app->menus[DEMO_MENU_VIEW] = (rm_menu){"View", app->view_items, 2};
    app->menus[DEMO_MENU_WINDOW] = (rm_menu){"Window", app->window_items, 2};
    app->menus[DEMO_MENU_HELP] = (rm_menu){"Help", app->help_items, 1};
}

wm_status demo_app_init(demo_app *app, ui_context *ui,
                        const wm_framebuffer *framebuffer)
{
    if (app == NULL || ui == NULL || framebuffer == NULL) return WM_ERROR_ARGUMENT;
    *app = (demo_app){.ui = ui, .colour = true, .running = true};
    init_menu_descriptors(app);
    wm_status status = ui_init(ui, framebuffer, app->menus, DEMO_MENU_COUNT);
    if (status != WM_OK) return status;
    const char *titles[] = {"FRAMEBUFFER", "INTERACTIONS", "PORTABLE C"};
    for (size_t i = 0; i < 3; ++i) {
        wm_id id;
        status = create_panel(app, initial_bounds[i], titles[i], (unsigned)i, &id);
        if (status != WM_OK) return status;
    }
    refresh_menu_state(app);
    return WM_OK;
}

wm_status demo_app_handle_event(demo_app *app, const wm_event *event)
{
    if (app == NULL || event == NULL || app->ui == NULL) return WM_ERROR_ARGUMENT;
    refresh_menu_state(app);
    ui_result result;
    wm_status status = ui_handle_event(app->ui, event, &result);
    if (status != WM_OK) return status;
    apply_window_update(app, &result.window);
    if (result.command != 0) {
        status = perform_action(app, result.command);
    } else if (!result.menu_consumed && event->type == WM_EVENT_KEY_DOWN) {
        switch (event->key) {
        case WM_KEY_N: status = perform_action(app, ACTION_NEW); break;
        case WM_KEY_C:
            status = perform_action(app, app->colour ? ACTION_MONOCHROME : ACTION_COLOUR);
            break;
        case WM_KEY_Q: status = perform_action(app, ACTION_QUIT); break;
        default: break;
        }
    }
    refresh_menu_state(app);
    return status;
}
