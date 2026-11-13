#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "sdl_host.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool check_sdl(int result, const char *operation)
{
    if (result < 0) {
        fprintf(stderr, "%s: %s\n", operation, SDL_GetError());
        return false;
    }
    return true;
}

bool sdl_host_init(sdl_host *host, const sdl_host_config *config)
{
    if (host == NULL || config == NULL || config->title == NULL ||
        config->width < 2 || config->height < 2 || config->width > INT_MAX / 4 ||
        (size_t)config->height > SIZE_MAX / sizeof(uint32_t) / (size_t)config->width) {
        fprintf(stderr, "Invalid SDL host configuration\n");
        return false;
    }
    *host = (sdl_host){.frame_delay_ms = config->frame_delay_ms};
    SDL_SetMainReady();
    if (!check_sdl(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), "initialize SDL")) {
        return false;
    }
    host->initialized = true;
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
    if (config->resizable) flags |= SDL_WINDOW_RESIZABLE;
    host->window = SDL_CreateWindow(config->title, SDL_WINDOWPOS_CENTERED,
                                    SDL_WINDOWPOS_CENTERED, config->width,
                                    config->height, flags);
    if (host->window == NULL) {
        fprintf(stderr, "Create host window: %s\n", SDL_GetError());
        goto fail;
    }
    SDL_SetWindowMinimumSize(host->window, config->width / 2, config->height / 2);
    host->renderer = SDL_CreateRenderer(host->window, -1, SDL_RENDERER_SOFTWARE);
    if (host->renderer == NULL) {
        fprintf(stderr, "Create renderer: %s\n", SDL_GetError());
        goto fail;
    }
    if (!check_sdl(SDL_RenderSetLogicalSize(host->renderer, config->width, config->height),
                   "set logical display size")) {
        goto fail;
    }
    host->texture = SDL_CreateTexture(host->renderer, SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      config->width, config->height);
    if (host->texture == NULL) {
        fprintf(stderr, "Create framebuffer texture: %s\n", SDL_GetError());
        goto fail;
    }
    if (!check_sdl(SDL_SetTextureBlendMode(host->texture, SDL_BLENDMODE_NONE),
                   "set framebuffer blend mode")) {
        goto fail;
    }
    host->framebuffer = (wm_framebuffer){
        calloc((size_t)config->width * (size_t)config->height, sizeof(uint32_t)),
        config->width, config->height, (size_t)config->width
    };
    if (host->framebuffer.pixels == NULL) {
        fprintf(stderr, "Unable to allocate the software framebuffer\n");
        goto fail;
    }
    return true;

fail:
    sdl_host_destroy(host);
    return false;
}

void sdl_host_destroy(sdl_host *host)
{
    assert(host != NULL);
    SDL_DestroyTexture(host->texture);
    SDL_DestroyRenderer(host->renderer);
    SDL_DestroyWindow(host->window);
    free(host->framebuffer.pixels);
    if (host->initialized) SDL_Quit();
    *host = (sdl_host){0};
}

static uint32_t translate_modifiers(SDL_Keymod modifiers)
{
    uint32_t result = 0;
    if ((modifiers & KMOD_SHIFT) != 0) result |= WM_MOD_SHIFT;
    if ((modifiers & KMOD_CTRL) != 0) result |= WM_MOD_CTRL;
    if ((modifiers & KMOD_ALT) != 0) result |= WM_MOD_ALT;
    if ((modifiers & KMOD_GUI) != 0) result |= WM_MOD_META;
    return result;
}

bool sdl_host_poll_event(sdl_host *host, sdl_host_event *output)
{
    assert(host != NULL && host->initialized && output != NULL);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        *output = (sdl_host_event){.type = SDL_HOST_INPUT};
        wm_event *input = &output->input;
        switch (event.type) {
        case SDL_QUIT:
            output->type = SDL_HOST_QUIT;
            return true;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                input->type = WM_EVENT_CANCEL;
                return true;
            }
            if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
                output->type = SDL_HOST_REPAINT;
                return true;
            }
            break;
        case SDL_MOUSEMOTION:
            input->type = WM_EVENT_POINTER_MOVE;
            input->x = event.motion.x;
            input->y = event.motion.y;
            return true;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            input->type = event.type == SDL_MOUSEBUTTONDOWN
                        ? WM_EVENT_POINTER_DOWN : WM_EVENT_POINTER_UP;
            input->x = event.button.x;
            input->y = event.button.y;
            switch (event.button.button) {
            case SDL_BUTTON_LEFT: input->button = WM_BUTTON_PRIMARY; break;
            case SDL_BUTTON_RIGHT: input->button = WM_BUTTON_SECONDARY; break;
            case SDL_BUTTON_MIDDLE: input->button = WM_BUTTON_MIDDLE; break;
            default: continue;
            }
            return true;
        case SDL_KEYDOWN:
            if (event.key.repeat != 0) break;
            input->type = WM_EVENT_KEY_DOWN;
            input->modifiers = translate_modifiers(event.key.keysym.mod);
            switch (event.key.keysym.sym) {
            case SDLK_q: input->key = WM_KEY_Q; break;
            case SDLK_c: input->key = WM_KEY_C; break;
            case SDLK_n: input->key = WM_KEY_N; break;
            case SDLK_TAB: input->key = WM_KEY_TAB; break;
            case SDLK_ESCAPE: input->key = WM_KEY_ESCAPE; break;
            case SDLK_F4: input->key = WM_KEY_F4; break;
            case SDLK_w: input->key = WM_KEY_W; break;
            case SDLK_LEFT: input->key = WM_KEY_LEFT; break;
            case SDLK_RIGHT: input->key = WM_KEY_RIGHT; break;
            case SDLK_UP: input->key = WM_KEY_UP; break;
            case SDLK_DOWN: input->key = WM_KEY_DOWN; break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER: input->key = WM_KEY_ENTER; break;
            case SDLK_SPACE: input->key = WM_KEY_SPACE; break;
            case SDLK_F10: input->key = WM_KEY_F10; break;
            default: continue;
            }
            return true;
        default:
            break;
        }
    }
    return false;
}

void sdl_host_request_capture(sdl_host *host, bool captured)
{
    assert(host != NULL && host->initialized);
    if (captured == host->capture_requested) return;
    host->capture_requested = captured;
    const char *driver = SDL_GetCurrentVideoDriver();
    if (driver != NULL && strcmp(driver, "dummy") == 0) return;
    if (SDL_CaptureMouse(captured ? SDL_TRUE : SDL_FALSE) != 0 &&
        !host->capture_warning) {
        fprintf(stderr, "Native pointer capture unavailable: %s\n", SDL_GetError());
        host->capture_warning = true;
    }
}

bool sdl_host_present(sdl_host *host, bool pixels_changed)
{
    assert(host != NULL && host->initialized);
    if (pixels_changed || !host->texture_valid) {
        if (!check_sdl(SDL_UpdateTexture(host->texture, NULL, host->framebuffer.pixels,
                                         (int)(host->framebuffer.stride_pixels *
                                               sizeof(uint32_t))),
                       "upload framebuffer")) {
            return false;
        }
        host->texture_valid = true;
    }
    if (!check_sdl(SDL_RenderClear(host->renderer), "clear host display") ||
        !check_sdl(SDL_RenderCopy(host->renderer, host->texture, NULL, NULL),
                   "present framebuffer")) {
        return false;
    }
    SDL_RenderPresent(host->renderer);
    if (host->frame_delay_ms != 0) SDL_Delay(host->frame_delay_ms);
    return true;
}

bool sdl_host_save_screenshot(sdl_host *host, const char *path)
{
    if (host == NULL || !host->initialized || path == NULL) {
        fprintf(stderr, "Invalid screenshot request\n");
        return false;
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(
        host->framebuffer.pixels, host->framebuffer.width, host->framebuffer.height,
        32, (int)(host->framebuffer.stride_pixels * sizeof(uint32_t)),
        SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL) {
        fprintf(stderr, "Create screenshot surface: %s\n", SDL_GetError());
        return false;
    }
    bool success = check_sdl(SDL_SaveBMP(surface, path), "save screenshot");
    SDL_FreeSurface(surface);
    return success;
}
