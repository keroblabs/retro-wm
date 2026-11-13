#include "retro_menu.h"
#include "retro_ui.h"
#include "retro_wm.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 320, HEIGHT = 240, STRIDE = WIDTH + 7 };
#define GUARD UINT32_C(0x12345678)

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __func__, __LINE__, #condition); \
        return false; \
    } \
} while (0)

#define CHECK_OK(expression) do { \
    wm_status status_ = (expression); \
    if (status_ != WM_OK) { \
        fprintf(stderr, "%s:%d: %s: %s\n", __func__, __LINE__, \
                #expression, wm_status_string(status_)); \
        return false; \
    } \
} while (0)

typedef struct {
    wm_context wm;
    uint32_t storage[STRIDE * HEIGHT + 2];
    wm_framebuffer framebuffer;
} fixture;

static bool setup(fixture *f)
{
    for (size_t i = 0; i < sizeof(f->storage) / sizeof(f->storage[0]); ++i) {
        f->storage[i] = GUARD;
    }
    f->framebuffer = (wm_framebuffer){f->storage + 1, WIDTH, HEIGHT, STRIDE};
    CHECK_OK(wm_init(&f->wm, WIDTH, HEIGHT));
    return true;
}

static void paint_solid(wm_canvas *canvas, int32_t width,
                        int32_t height, void *userdata)
{
    const wm_color *color = userdata;
    wm_canvas_fill(canvas, (wm_rect){0, 0, width, height}, *color);
}

static wm_status add_window(fixture *f, wm_rect bounds, wm_color *color, wm_id *id)
{
    wm_window_desc desc = {
        "TEST", bounds, WM_MIN_WIDTH, WM_MIN_HEIGHT, paint_solid, color
    };
    return wm_create_window(&f->wm, &desc, id);
}

static wm_status pointer(fixture *f, wm_event_type type, int32_t x, int32_t y)
{
    wm_event event = {
        .type = type, .x = x, .y = y, .button = WM_BUTTON_PRIMARY
    };
    return wm_handle_event(&f->wm, &event, NULL);
}

static wm_status key(fixture *f, wm_key code, uint32_t modifiers)
{
    wm_event event = {
        .type = WM_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers
    };
    return wm_handle_event(&f->wm, &event, NULL);
}

static uint32_t pixel(const fixture *f, int32_t x, int32_t y)
{
    return f->framebuffer.pixels[(size_t)y * STRIDE + (size_t)x];
}

static bool focus_and_hit_testing(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color red = WM_RGB(180, 30, 30), blue = WM_RGB(30, 30, 180);
    wm_id a, b;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 200, 160}, &red, &a));
    CHECK_OK(add_window(&f, (wm_rect){70, 60, 200, 160}, &blue, &b));
    wm_hit hit;
    CHECK_OK(wm_hit_test(&f.wm, 100, 110, &hit));
    CHECK(hit.window == b && hit.part == WM_HIT_CLIENT);
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 100, 110) == blue);
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 30, 70));
    CHECK(wm_front_window(&f.wm) == a && !wm_has_capture(&f.wm));
    CHECK_OK(wm_hit_test(&f.wm, 100, 110, &hit));
    CHECK(hit.window == a);
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 100, 110) == red);
    CHECK(wm_window_count(&f.wm) == 2);
    return true;
}

static bool dragging_captures_and_clamps(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(80, 90, 100);
    wm_id id;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 130}, &color, &id));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 60, 30));
    CHECK(wm_has_capture(&f.wm));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_MOVE, INT32_MIN, INT32_MAX));
    wm_rect bounds;
    CHECK_OK(wm_get_window_bounds(&f.wm, id, &bounds));
    CHECK(bounds.x == 20 && bounds.y == 20);
    wm_event secondary_release = {
        .type = WM_EVENT_POINTER_UP, .button = WM_BUTTON_SECONDARY
    };
    CHECK_OK(wm_handle_event(&f.wm, &secondary_release, NULL));
    CHECK(wm_has_capture(&f.wm));
    wm_event release = {
        .type = WM_EVENT_POINTER_UP, .button = WM_BUTTON_PRIMARY,
        .x = INT32_MIN, .y = INT32_MAX
    };
    wm_update update;
    CHECK_OK(wm_handle_event(&f.wm, &release, &update));
    CHECK((update.changes & WM_UPDATE_MOVED) != 0 && update.window == id);
    CHECK(!wm_has_capture(&f.wm));
    CHECK_OK(wm_get_window_bounds(&f.wm, id, &bounds));
    CHECK(bounds.x == 0 && bounds.y == HEIGHT - bounds.height);
    return true;
}

static bool cancelled_interactions_leave_no_pixels(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(60, 100, 130);
    wm_id id;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 130}, &color, &id));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    uint32_t before[STRIDE * HEIGHT + 2];
    memcpy(before, f.storage, sizeof(before));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 60, 30));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_MOVE, 130, 100));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(memcmp(before, f.storage, sizeof(before)) != 0);
    CHECK_OK(key(&f, WM_KEY_ESCAPE, 0));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(memcmp(before, f.storage, sizeof(before)) == 0);
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 190, 140));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_MOVE, 250, 190));
    wm_event cancel = {.type = WM_EVENT_CANCEL};
    CHECK_OK(wm_handle_event(&f.wm, &cancel, NULL));
    CHECK(!wm_has_capture(&f.wm));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(memcmp(before, f.storage, sizeof(before)) == 0);
    return true;
}

static bool resizing_obeys_minimum_and_display_bounds(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(100, 100, 100);
    wm_id id;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 130}, &color, &id));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 190, 140));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_UP, INT32_MIN, INT32_MIN));
    wm_rect bounds;
    CHECK_OK(wm_get_window_bounds(&f.wm, id, &bounds));
    CHECK(bounds.width == WM_MIN_WIDTH && bounds.height == WM_MIN_HEIGHT);
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 108, 76));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_UP, INT32_MAX, INT32_MAX));
    CHECK_OK(wm_get_window_bounds(&f.wm, id, &bounds));
    CHECK(bounds.x == 20 && bounds.y == 20);
    CHECK(bounds.width == WIDTH - 20 && bounds.height == HEIGHT - 20);
    return true;
}

static bool close_tracks_release_and_rejects_stale_ids(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(120, 120, 120);
    wm_id a, b, replacement;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 140}, &color, &a));
    CHECK_OK(add_window(&f, (wm_rect){70, 60, 180, 150}, &color, &b));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 232, 70));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_MOVE, 0, 0));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_UP, 0, 0));
    CHECK(wm_window_count(&f.wm) == 2);
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 232, 70));
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_UP, 232, 70));
    CHECK(wm_window_count(&f.wm) == 1 && wm_front_window(&f.wm) == a);
    wm_rect bounds;
    CHECK(wm_get_window_bounds(&f.wm, b, &bounds) == WM_ERROR_NOT_FOUND);
    CHECK_OK(add_window(&f, (wm_rect){70, 60, 180, 150}, &color, &replacement));
    CHECK(replacement != b);
    CHECK(wm_close_window(&f.wm, b) == WM_ERROR_NOT_FOUND);
    CHECK(wm_front_window(&f.wm) == replacement);
    return true;
}

static bool keyboard_focus_and_close(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(130, 140, 150);
    wm_id a, b;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 130}, &color, &a));
    CHECK_OK(add_window(&f, (wm_rect){70, 60, 180, 150}, &color, &b));
    CHECK_OK(key(&f, WM_KEY_TAB, 0));
    CHECK(wm_front_window(&f.wm) == a);
    CHECK_OK(key(&f, WM_KEY_TAB, WM_MOD_SHIFT));
    CHECK(wm_front_window(&f.wm) == b);
    CHECK_OK(pointer(&f, WM_EVENT_POINTER_DOWN, 110, 70));
    CHECK(wm_has_capture(&f.wm));
    CHECK_OK(key(&f, WM_KEY_W, WM_MOD_CTRL));
    CHECK(wm_front_window(&f.wm) == a && !wm_has_capture(&f.wm));
    CHECK_OK(key(&f, WM_KEY_F4, WM_MOD_ALT));
    CHECK(wm_window_count(&f.wm) == 0);
    CHECK_OK(key(&f, WM_KEY_TAB, 0));
    return true;
}

static bool closing_repaints_exposed_content(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color red = WM_RGB(190, 40, 40), blue = WM_RGB(40, 40, 190);
    wm_id a, b;
    CHECK_OK(add_window(&f, (wm_rect){20, 20, 200, 170}, &red, &a));
    CHECK_OK(add_window(&f, (wm_rect){70, 60, 200, 150}, &blue, &b));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 100, 110) == blue);
    CHECK_OK(wm_close_window(&f.wm, b));
    CHECK(wm_needs_repaint(&f.wm));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 100, 110) == red);
    CHECK(!wm_needs_repaint(&f.wm));
    CHECK_OK(wm_close_window(&f.wm, a));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 100, 110) == f.wm.theme.desktop);
    return true;
}

static void paint_outside_client(wm_canvas *canvas, int32_t width,
                                 int32_t height, void *userdata)
{
    (void)width;
    (void)height;
    const wm_color *color = userdata;
    wm_canvas_fill(canvas, (wm_rect){-100, -100, INT32_MAX, INT32_MAX}, *color);
    wm_canvas_frame(canvas, (wm_rect){INT32_MAX, INT32_MAX, 50, 50}, *color);
    wm_canvas_text(canvas, INT32_MAX, INT32_MAX, "OFFSCREEN", *color);
}

static bool client_clipping_stride_and_colour(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(70, 180, 100);
    wm_window_desc desc = {
        "CLIPPING", {20, 20, 180, 140}, WM_MIN_WIDTH, WM_MIN_HEIGHT,
        paint_outside_client, &color
    };
    wm_id id;
    CHECK_OK(wm_create_window(&f.wm, &desc, &id));
    wm_theme theme = wm_theme_colour();
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 0, 0) == theme.desktop);
    CHECK(pixel(&f, 20, 20) == theme.border);
    CHECK(pixel(&f, 24, 24) == theme.active_title);
    CHECK(pixel(&f, 30, 70) == color);
    CHECK(f.storage[0] == GUARD);
    CHECK(f.storage[STRIDE * HEIGHT + 1] == GUARD);
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = WIDTH; x < STRIDE; ++x) {
            CHECK(f.framebuffer.pixels[(size_t)y * STRIDE + (size_t)x] == GUARD);
        }
    }
    wm_theme monochrome = wm_theme_monochrome();
    CHECK_OK(wm_set_theme(&f.wm, &monochrome));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK(pixel(&f, 0, 0) == monochrome.desktop);
    CHECK(pixel(&f, 24, 24) == monochrome.active_title);
    CHECK(pixel(&f, 30, 70) == color);
    return true;
}

typedef struct {
    rm_context menu;
    rm_item file_items[4], view_items[2];
    rm_menu menus[2];
} menu_fixture;

static bool setup_menu(menu_fixture *f)
{
    f->file_items[0] = (rm_item){RM_ITEM_COMMAND, "New", "N", 101, true, false};
    f->file_items[1] = (rm_item){.type = RM_ITEM_SEPARATOR};
    f->file_items[2] = (rm_item){RM_ITEM_COMMAND, "Disabled", NULL, 102, false, false};
    f->file_items[3] = (rm_item){RM_ITEM_COMMAND, "Checked", NULL, 103, true, true};
    f->view_items[0] = (rm_item){RM_ITEM_COMMAND, "Colour", NULL, 201, true, true};
    f->view_items[1] = (rm_item){RM_ITEM_COMMAND, "Monochrome", NULL, 202, true, false};
    f->menus[0] = (rm_menu){"File", f->file_items, 4};
    f->menus[1] = (rm_menu){"View", f->view_items, 2};
    CHECK_OK(rm_init(&f->menu, WIDTH, HEIGHT, f->menus, 2));
    return true;
}

static wm_status menu_pointer(menu_fixture *f, wm_event_type type,
                              wm_rect target, rm_result *result)
{
    wm_event event = {
        .type = type, .button = WM_BUTTON_PRIMARY,
        .x = target.x + target.width / 2, .y = target.y + target.height / 2
    };
    return rm_handle_event(&f->menu, &event, result);
}

static wm_status menu_key(menu_fixture *f, wm_key key_code, rm_result *result)
{
    wm_event event = {.type = WM_EVENT_KEY_DOWN, .key = key_code};
    return rm_handle_event(&f->menu, &event, result);
}

static bool menu_mouse_click_and_drag_selection(void)
{
    menu_fixture f;
    CHECK(setup_menu(&f));
    rm_menu_bounds file, view;
    wm_rect item;
    rm_result result;
    CHECK_OK(rm_get_menu_bounds(&f.menu, 0, &file));
    CHECK_OK(rm_get_menu_bounds(&f.menu, 1, &view));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, file.button, &result));
    CHECK(result.consumed && rm_is_open(&f.menu));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, file.button, &result));
    CHECK(result.command == 0 && rm_is_open(&f.menu));
    CHECK_OK(rm_get_item_bounds(&f.menu, 0, 0, &item));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, item, &result));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, item, &result));
    CHECK(result.consumed && result.command == 101 && !rm_is_open(&f.menu));

    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, file.button, &result));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_MOVE, view.button, &result));
    CHECK_OK(rm_get_item_bounds(&f.menu, 1, 1, &item));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_MOVE, item, &result));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, item, &result));
    CHECK(result.command == 202 && !rm_is_open(&f.menu));
    return true;
}

static bool menu_disabled_items_and_dismissal(void)
{
    menu_fixture f;
    CHECK(setup_menu(&f));
    rm_menu_bounds bounds;
    wm_rect row;
    rm_result result;
    CHECK_OK(rm_get_menu_bounds(&f.menu, 0, &bounds));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, bounds.button, &result));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, bounds.button, &result));
    for (size_t i = 1; i <= 2; ++i) {
        CHECK_OK(rm_get_item_bounds(&f.menu, 0, i, &row));
        CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, row, &result));
        CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, row, &result));
        CHECK(result.consumed && result.command == 0 && rm_is_open(&f.menu));
    }
    CHECK_OK(menu_key(&f, WM_KEY_N, &result));
    CHECK(result.consumed && result.command == 0);
    wm_event secondary_release = {
        .type = WM_EVENT_POINTER_UP, .button = WM_BUTTON_SECONDARY,
        .x = bounds.popup.x + 30, .y = bounds.popup.y + 10
    };
    CHECK_OK(rm_handle_event(&f.menu, &secondary_release, &result));
    CHECK(result.command == 0 && rm_is_open(&f.menu));
    wm_rect outside = {WIDTH - 2, HEIGHT - 2, 1, 1};
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_DOWN, outside, &result));
    CHECK(result.consumed && result.command == 0 && !rm_is_open(&f.menu));
    CHECK_OK(menu_pointer(&f, WM_EVENT_POINTER_UP, outside, &result));
    CHECK(!result.consumed);
    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    wm_event cancel = {.type = WM_EVENT_CANCEL};
    CHECK_OK(rm_handle_event(&f.menu, &cancel, &result));
    CHECK(result.consumed && !rm_is_open(&f.menu));
    return true;
}

static bool menu_keyboard_navigation_skips_unavailable_items(void)
{
    menu_fixture f;
    CHECK(setup_menu(&f));
    rm_result result;
    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    CHECK_OK(menu_key(&f, WM_KEY_DOWN, &result));
    CHECK_OK(menu_key(&f, WM_KEY_ENTER, &result));
    CHECK(result.command == 103 && !rm_is_open(&f.menu));
    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    CHECK_OK(menu_key(&f, WM_KEY_UP, &result));
    CHECK_OK(menu_key(&f, WM_KEY_DOWN, &result));
    CHECK_OK(menu_key(&f, WM_KEY_RIGHT, &result));
    CHECK_OK(menu_key(&f, WM_KEY_DOWN, &result));
    CHECK_OK(menu_key(&f, WM_KEY_ENTER, &result));
    CHECK(result.command == 202);
    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    CHECK_OK(menu_key(&f, WM_KEY_RIGHT, &result));
    CHECK_OK(menu_key(&f, WM_KEY_LEFT, &result));
    CHECK_OK(menu_key(&f, WM_KEY_SPACE, &result));
    CHECK(result.command == 101);

    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    f.file_items[0].enabled = false;
    f.file_items[3].enabled = false;
    rm_invalidate(&f.menu);
    CHECK_OK(menu_key(&f, WM_KEY_DOWN, &result));
    CHECK_OK(menu_key(&f, WM_KEY_ENTER, &result));
    CHECK(result.command == 0 && rm_is_open(&f.menu));
    CHECK_OK(menu_key(&f, WM_KEY_RIGHT, &result));
    CHECK_OK(menu_key(&f, WM_KEY_ENTER, &result));
    CHECK(result.command == 201);
    CHECK_OK(menu_key(&f, WM_KEY_F10, &result));
    CHECK_OK(menu_key(&f, WM_KEY_ESCAPE, &result));
    CHECK(result.command == 0 && !rm_is_open(&f.menu));
    return true;
}

static bool menu_rendering_and_dismissal_restore_the_scene(void)
{
    fixture f;
    menu_fixture menus;
    CHECK(setup(&f) && setup_menu(&menus));
    wm_color color = WM_RGB(74, 124, 184);
    wm_id id;
    CHECK_OK(add_window(&f, (wm_rect){0, 30, 300, 180}, &color, &id));
    wm_theme theme = wm_theme_colour();
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK_OK(rm_render(&menus.menu, &f.framebuffer, &theme));
    uint32_t before[STRIDE * HEIGHT + 2];
    memcpy(before, f.storage, sizeof(before));
    rm_result result;
    CHECK_OK(menu_key(&menus, WM_KEY_F10, &result));
    CHECK(rm_needs_repaint(&menus.menu));
    CHECK_OK(rm_render(&menus.menu, &f.framebuffer, &theme));
    CHECK(!rm_needs_repaint(&menus.menu));
    CHECK(memcmp(before, f.storage, sizeof(before)) != 0);
    wm_rect row;
    CHECK_OK(rm_get_item_bounds(&menus.menu, 0, 0, &row));
    CHECK(pixel(&f, row.x + 1, row.y + 1) == theme.active_title);
    CHECK_OK(rm_get_item_bounds(&menus.menu, 0, 3, &row));
    int32_t check_x = row.x + 6;
    int32_t check_y = row.y + (row.height - WM_TEXT_HEIGHT) / 2 + 5;
    CHECK(pixel(&f, check_x, check_y) == theme.border);
    menus.file_items[3].checked = false;
    rm_invalidate(&menus.menu);
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK_OK(rm_render(&menus.menu, &f.framebuffer, &theme));
    CHECK(pixel(&f, check_x, check_y) == theme.client);
    CHECK_OK(menu_key(&menus, WM_KEY_ESCAPE, &result));
    CHECK_OK(wm_render(&f.wm, &f.framebuffer));
    CHECK_OK(rm_render(&menus.menu, &f.framebuffer, &theme));
    CHECK(memcmp(before, f.storage, sizeof(before)) == 0);
    CHECK(f.storage[0] == GUARD && f.storage[STRIDE * HEIGHT + 1] == GUARD);
    for (int32_t y = 0; y < HEIGHT; ++y) {
        for (int32_t x = WIDTH; x < STRIDE; ++x) {
            CHECK(f.framebuffer.pixels[(size_t)y * STRIDE + (size_t)x] == GUARD);
        }
    }
    return true;
}

static bool menu_layout_and_invalid_descriptors(void)
{
    menu_fixture menus;
    fixture f;
    CHECK(setup_menu(&menus) && setup(&f));
    menus.view_items[1].label = "A VERY LONG MENU ITEM THAT NEEDS CLIPPING";
    CHECK_OK(rm_init(&menus.menu, WIDTH, HEIGHT, menus.menus, 2));
    rm_menu_bounds bounds;
    CHECK_OK(rm_get_menu_bounds(&menus.menu, 1, &bounds));
    CHECK(bounds.popup.x >= 0 && bounds.popup.x + bounds.popup.width <= WIDTH);
    rm_result result;
    CHECK_OK(menu_pointer(&menus, WM_EVENT_POINTER_DOWN, bounds.button, &result));
    wm_theme theme = wm_theme_monochrome();
    CHECK_OK(rm_render(&menus.menu, &f.framebuffer, &theme));
    CHECK_OK(menu_key(&menus, WM_KEY_ESCAPE, &result));
    CHECK(rm_init(&menus.menu, WIDTH, 80, menus.menus, 2) == WM_ERROR_ARGUMENT);
    CHECK(rm_init(&menus.menu, WIDTH, HEIGHT, menus.menus, RM_MAX_MENUS + 1) ==
          WM_ERROR_ARGUMENT);
    menus.file_items[0].command = 0;
    CHECK(rm_init(&menus.menu, WIDTH, HEIGHT, menus.menus, 2) == WM_ERROR_ARGUMENT);
    menus.file_items[0].command = 101;
    CHECK_OK(menu_key(&menus, WM_KEY_F10, &result));
    CHECK_OK(menu_key(&menus, WM_KEY_ENTER, &result));
    CHECK(result.command == 101);
    return true;
}

static bool ui_routes_capture_and_returns_application_commands(void)
{
    fixture f;
    menu_fixture menus;
    ui_context ui;
    CHECK(setup(&f) && setup_menu(&menus));
    CHECK_OK(ui_init(&ui, &f.framebuffer, menus.menus, 2));
    wm_color color = WM_RGB(74, 124, 184);
    wm_window_desc desc = {
        "CLIENT", {20, 20, 180, 130}, WM_MIN_WIDTH, WM_MIN_HEIGHT,
        paint_solid, &color
    };
    wm_id id;
    CHECK_OK(wm_create_window(&ui.windows, &desc, &id));
    ui_result result;
    wm_event event = {
        .type = WM_EVENT_POINTER_DOWN, .button = WM_BUTTON_PRIMARY,
        .x = 60, .y = 30 + RM_BAR_HEIGHT
    };
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(!result.menu_consumed && ui_has_capture(&ui));
    event.type = WM_EVENT_POINTER_MOVE;
    event.y = RM_BAR_HEIGHT / 2;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    event.type = WM_EVENT_POINTER_UP;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK((result.window.changes & WM_UPDATE_MOVED) != 0);
    CHECK(!ui_has_capture(&ui));
    wm_rect bounds;
    CHECK_OK(wm_get_window_bounds(&ui.windows, id, &bounds));
    CHECK(bounds.y == 0);
    event = (wm_event){.type = WM_EVENT_KEY_DOWN, .key = WM_KEY_F10};
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(result.menu_consumed && ui_has_capture(&ui));
    event.key = WM_KEY_N;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(result.menu_consumed && result.command == 0);
    event.key = WM_KEY_ENTER;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(result.menu_consumed && result.command == 101);
    CHECK(wm_window_count(&ui.windows) == 1 && !ui_has_capture(&ui));
    return true;
}

static bool ui_cancels_interactions_and_reports_closed_windows(void)
{
    fixture f;
    menu_fixture menus;
    ui_context ui;
    CHECK(setup(&f) && setup_menu(&menus));
    CHECK_OK(ui_init(&ui, &f.framebuffer, menus.menus, 2));
    wm_window_desc desc = {
        "CLIENT", {20, 20, 180, 130}, WM_MIN_WIDTH, WM_MIN_HEIGHT, NULL, NULL
    };
    wm_id first, second;
    CHECK_OK(wm_create_window(&ui.windows, &desc, &first));
    desc.bounds = (wm_rect){70, 50, 180, 130};
    CHECK_OK(wm_create_window(&ui.windows, &desc, &second));
    wm_event event = {
        .type = WM_EVENT_POINTER_DOWN, .button = WM_BUTTON_PRIMARY,
        .x = 60, .y = 30 + RM_BAR_HEIGHT
    };
    ui_result result;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    event.type = WM_EVENT_POINTER_MOVE;
    event.x = 120;
    event.y = 90 + RM_BAR_HEIGHT;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    event = (wm_event){.type = WM_EVENT_KEY_DOWN, .key = WM_KEY_F10};
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(result.menu_consumed && !wm_has_capture(&ui.windows));
    wm_rect bounds;
    CHECK_OK(wm_get_window_bounds(&ui.windows, first, &bounds));
    CHECK(bounds.x == 20 && bounds.y == 20);
    event = (wm_event){.type = WM_EVENT_CANCEL};
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(!ui_has_capture(&ui));
    event = (wm_event){
        .type = WM_EVENT_KEY_DOWN, .key = WM_KEY_W, .modifiers = WM_MOD_CTRL
    };
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(result.window.window == first);
    CHECK((result.window.changes & WM_UPDATE_CLOSED) != 0);
    CHECK(wm_front_window(&ui.windows) == second);
    return true;
}

static bool ui_composes_framebuffer_views_and_preserves_valid_state(void)
{
    fixture f;
    menu_fixture menus;
    ui_context ui;
    CHECK(setup(&f) && setup_menu(&menus));
    CHECK_OK(ui_init(&ui, &f.framebuffer, menus.menus, 2));
    wm_color color = WM_RGB(60, 130, 180);
    wm_window_desc desc = {
        "CLIENT", {0, 0, 180, 130}, WM_MIN_WIDTH, WM_MIN_HEIGHT, paint_solid, &color
    };
    wm_id id;
    CHECK_OK(wm_create_window(&ui.windows, &desc, &id));
    CHECK_OK(ui_render(&ui));
    wm_theme theme = wm_theme_colour();
    CHECK(pixel(&f, 0, 0) == theme.client);
    CHECK(pixel(&f, 0, RM_BAR_HEIGHT) == theme.border);
    CHECK(pixel(&f, 10, RM_BAR_HEIGHT + WM_BORDER + WM_TITLE_HEIGHT + 3) == color);
    uint32_t before[STRIDE * HEIGHT + 2];
    memcpy(before, f.storage, sizeof(before));
    ui_result result;
    wm_event event = {.type = WM_EVENT_KEY_DOWN, .key = WM_KEY_F10};
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK_OK(ui_render(&ui));
    CHECK(memcmp(before, f.storage, sizeof(before)) != 0);
    event.key = WM_KEY_ESCAPE;
    CHECK_OK(ui_handle_event(&ui, &event, &result));
    CHECK(ui_needs_repaint(&ui));
    CHECK_OK(ui_render(&ui));
    CHECK(memcmp(before, f.storage, sizeof(before)) == 0);
    wm_framebuffer invalid = f.framebuffer;
    invalid.stride_pixels = WIDTH - 1;
    CHECK(ui_init(&ui, &invalid, menus.menus, 2) == WM_ERROR_ARGUMENT);
    invalid = f.framebuffer;
    invalid.height = WM_MIN_HEIGHT + RM_BAR_HEIGHT - 1;
    CHECK(ui_init(&ui, &invalid, menus.menus, 2) == WM_ERROR_ARGUMENT);
    CHECK(ui_init(&ui, &f.framebuffer, menus.menus, 0) == WM_ERROR_ARGUMENT);
    CHECK_OK(ui_render(&ui));
    CHECK(memcmp(before, f.storage, sizeof(before)) == 0);
    return true;
}

static bool invalid_requests_are_reported(void)
{
    fixture f;
    CHECK(setup(&f));
    wm_color color = WM_RGB(0, 0, 0);
    wm_id id = WM_NO_WINDOW;
    CHECK(wm_init(NULL, WIDTH, HEIGHT) == WM_ERROR_ARGUMENT);
    CHECK(add_window(&f, (wm_rect){-1, 0, 180, 140}, &color, &id) == WM_ERROR_ARGUMENT);
    CHECK(wm_window_count(&f.wm) == 0 && id == WM_NO_WINDOW);
    wm_framebuffer invalid = f.framebuffer;
    invalid.stride_pixels = WIDTH - 1;
    CHECK(wm_render(&f.wm, &invalid) == WM_ERROR_ARGUMENT);
    CHECK(f.storage[1] == GUARD);
    wm_event invalid_button = {
        .type = WM_EVENT_POINTER_DOWN, .button = WM_BUTTON_NONE
    };
    CHECK(wm_handle_event(&f.wm, &invalid_button, NULL) == WM_ERROR_ARGUMENT);
    for (size_t i = 0; i < WM_MAX_WINDOWS; ++i) {
        CHECK_OK(add_window(&f, (wm_rect){20, 20, 180, 140}, &color, &id));
    }
    wm_id last_id = id;
    CHECK(add_window(&f, (wm_rect){20, 20, 180, 140}, &color, &id) == WM_ERROR_CAPACITY);
    CHECK(id == last_id && wm_window_count(&f.wm) == WM_MAX_WINDOWS);
    return true;
}

int main(void)
{
    const struct {
        const char *name;
        bool (*run)(void);
    } tests[] = {
        {"focus and hit testing", focus_and_hit_testing},
        {"drag capture and clamping", dragging_captures_and_clamps},
        {"cancel restores pixels", cancelled_interactions_leave_no_pixels},
        {"resize limits", resizing_obeys_minimum_and_display_bounds},
        {"close tracking and stale IDs", close_tracks_release_and_rejects_stale_ids},
        {"keyboard focus and close", keyboard_focus_and_close},
        {"exposed-window repaint", closing_repaints_exposed_content},
        {"client clipping, stride and colour", client_clipping_stride_and_colour},
        {"invalid requests and capacity", invalid_requests_are_reported},
        {"menu mouse click and drag", menu_mouse_click_and_drag_selection},
        {"menu disabled items and dismissal", menu_disabled_items_and_dismissal},
        {"menu keyboard navigation", menu_keyboard_navigation_skips_unavailable_items},
        {"menu rendering and repaint", menu_rendering_and_dismissal_restore_the_scene},
        {"menu layout and validation", menu_layout_and_invalid_descriptors},
        {"UI capture routing and commands", ui_routes_capture_and_returns_application_commands},
        {"UI cancellation and close notifications", ui_cancels_interactions_and_reports_closed_windows},
        {"UI composition and validation", ui_composes_framebuffer_views_and_preserves_valid_state}
    };
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        if (!tests[i].run()) {
            fprintf(stderr, "FAIL: %s\n", tests[i].name);
            return 1;
        }
        printf("PASS: %s\n", tests[i].name);
    }
    return 0;
}
