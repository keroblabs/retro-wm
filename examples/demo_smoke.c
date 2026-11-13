#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "demo_smoke.h"

#include <stdio.h>

static bool check_wm(wm_status status, const char *operation)
{
    if (status != WM_OK) {
        fprintf(stderr, "%s: %s\n", operation, wm_status_string(status));
        return false;
    }
    return true;
}

static bool record_initial_windows(demo_smoke *smoke, const ui_context *ui)
{
    wm_hit first, second;
    if (!check_wm(wm_hit_test(&ui->windows, 160, 76, &first), "locate first window") ||
        !check_wm(wm_hit_test(&ui->windows, 370, 217, &second), "locate second window")) {
        return false;
    }
    smoke->initial_ids[0] = first.window;
    smoke->initial_ids[1] = second.window;
    smoke->initial_ids[2] = wm_front_window(&ui->windows);
    for (size_t i = 0; i < 3; ++i) {
        if (!check_wm(wm_get_window_bounds(&ui->windows, smoke->initial_ids[i],
                                           &smoke->initial_bounds[i]),
                      "record initial window bounds")) {
            return false;
        }
    }
    return true;
}

bool demo_smoke_enqueue(demo_smoke *smoke, const sdl_host *host,
                         const ui_context *ui, uint64_t frame)
{
    if (frame == 0 && !record_initial_windows(smoke, ui)) return false;
    if (frame == 8) smoke->created = wm_front_window(&ui->windows);
    SDL_Event event = {0};
    wm_rect first = smoke->initial_bounds[0];
    if (frame == 0 || frame == 2) {
        event.type = frame == 0 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        event.button.windowID = SDL_GetWindowID(host->window);
        event.button.button = SDL_BUTTON_LEFT;
        event.button.state = frame == 0 ? SDL_PRESSED : SDL_RELEASED;
        event.button.x = first.x + (frame == 0 ? 100 : 200);
        event.button.y = first.y + (frame == 0 ? 12 : 72) + RM_BAR_HEIGHT;
    } else if (frame == 1) {
        event.type = SDL_MOUSEMOTION;
        event.motion.windowID = SDL_GetWindowID(host->window);
        event.motion.state = SDL_BUTTON_LMASK;
        event.motion.x = first.x + 200;
        event.motion.y = first.y + 72 + RM_BAR_HEIGHT;
    } else if (frame >= 5 && frame <= 7) {
        rm_menu_bounds menu_bounds;
        wm_rect item;
        if (!check_wm(rm_get_menu_bounds(&ui->menus, 0, &menu_bounds),
                      "locate smoke-test menu") ||
            !check_wm(rm_get_item_bounds(&ui->menus, 0, 0, &item),
                      "locate smoke-test item")) {
            return false;
        }
        if (frame == 6) {
            event.type = SDL_MOUSEMOTION;
            event.motion.windowID = SDL_GetWindowID(host->window);
            event.motion.state = SDL_BUTTON_LMASK;
            event.motion.x = item.x + item.width / 2;
            event.motion.y = item.y + item.height / 2;
        } else {
            wm_rect target = frame == 5 ? menu_bounds.button : item;
            event.type = frame == 5 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            event.button.windowID = SDL_GetWindowID(host->window);
            event.button.button = SDL_BUTTON_LEFT;
            event.button.state = frame == 5 ? SDL_PRESSED : SDL_RELEASED;
            event.button.x = target.x + target.width / 2;
            event.button.y = target.y + target.height / 2;
        }
    } else if (frame == 16 || frame == 17) {
        event.type = frame == 16 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        event.button.windowID = SDL_GetWindowID(host->window);
        event.button.button = SDL_BUTTON_LEFT;
        event.button.state = frame == 16 ? SDL_PRESSED : SDL_RELEASED;
        event.button.x = smoke->initial_bounds[2].x + 230;
        event.button.y = smoke->initial_bounds[2].y + 42 + RM_BAR_HEIGHT;
    } else if (frame >= 18 && frame <= 20) {
        wm_rect bounds;
        if (!check_wm(wm_get_window_bounds(&ui->windows, smoke->created, &bounds),
                      "locate captured smoke-test window")) {
            return false;
        }
        if (frame == 19) {
            event.type = SDL_MOUSEMOTION;
            event.motion.windowID = SDL_GetWindowID(host->window);
            event.motion.state = SDL_BUTTON_LMASK;
            event.motion.x = bounds.x + 100;
            event.motion.y = RM_BAR_HEIGHT / 2;
        } else {
            event.type = frame == 18 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
            event.button.windowID = SDL_GetWindowID(host->window);
            event.button.button = SDL_BUTTON_LEFT;
            event.button.state = frame == 18 ? SDL_PRESSED : SDL_RELEASED;
            event.button.x = bounds.x + 100;
            event.button.y = frame == 18 ? bounds.y + RM_BAR_HEIGHT + 12
                                        : RM_BAR_HEIGHT / 2;
        }
    } else if (frame == 3 || frame == 4 || (frame >= 8 && frame <= 15) ||
               frame == 21 || frame == 22) {
        event.type = SDL_KEYDOWN;
        event.key.windowID = SDL_GetWindowID(host->window);
        event.key.state = SDL_PRESSED;
        switch (frame) {
        case 3: event.key.keysym.sym = SDLK_TAB; break;
        case 4: event.key.keysym.sym = SDLK_w; break;
        case 9:
        case 13: event.key.keysym.sym = SDLK_RIGHT; break;
        case 10:
        case 22: event.key.keysym.sym = SDLK_DOWN; break;
        case 11:
        case 14: event.key.keysym.sym = SDLK_RETURN; break;
        default: event.key.keysym.sym = SDLK_F10; break;
        }
        event.key.keysym.mod = frame == 4 ? KMOD_CTRL : KMOD_NONE;
    } else {
        return true;
    }
    if (SDL_PushEvent(&event) != 1) {
        fprintf(stderr, "Push smoke-test event: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

bool demo_smoke_verify(const demo_smoke *smoke, const ui_context *ui)
{
    wm_rect bounds, closed, captured;
    if (!check_wm(wm_get_window_bounds(&ui->windows, smoke->initial_ids[0], &bounds),
                  "inspect dragged window") ||
        !check_wm(wm_get_window_bounds(&ui->windows, smoke->created, &captured),
                  "inspect window released over menu bar")) {
        return false;
    }
    wm_theme colour = wm_theme_colour();
    if (bounds.x != smoke->initial_bounds[0].x + 100 ||
        bounds.y != smoke->initial_bounds[0].y + 60 ||
        bounds.width != smoke->initial_bounds[0].width ||
        bounds.height != smoke->initial_bounds[0].height ||
        wm_window_count(&ui->windows) != 3 ||
        wm_front_window(&ui->windows) != smoke->created ||
        wm_get_window_bounds(&ui->windows, smoke->initial_ids[1], &closed) != WM_ERROR_NOT_FOUND ||
        captured.y != 0 || wm_has_capture(&ui->windows) ||
        ui->windows.theme.active_title != colour.active_title ||
        !rm_is_open(&ui->menus)) {
        fprintf(stderr, "SDL smoke test failed: unexpected window/input state\n");
        return false;
    }
    puts("SDL smoke test passed: windows, menus, themes, no click-through, capture over menu bar");
    return true;
}
