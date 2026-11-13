#ifndef SDL_HOST_H
#define SDL_HOST_H

#include "retro_types.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

typedef struct {
    const char *title;
    int32_t width, height;
    uint32_t frame_delay_ms;
    bool resizable;
} sdl_host_config;

typedef enum {
    SDL_HOST_INPUT,
    SDL_HOST_QUIT,
    SDL_HOST_REPAINT
} sdl_host_event_type;

typedef struct {
    sdl_host_event_type type;
    wm_event input;
} sdl_host_event;

typedef struct {
    struct SDL_Window *window;
    struct SDL_Renderer *renderer;
    struct SDL_Texture *texture;
    wm_framebuffer framebuffer;
    uint32_t frame_delay_ms;
    bool initialized, texture_valid, capture_requested, capture_warning;
} sdl_host;

bool sdl_host_init(sdl_host *host, const sdl_host_config *config);
void sdl_host_destroy(sdl_host *host);
bool sdl_host_poll_event(sdl_host *host, sdl_host_event *event);
void sdl_host_request_capture(sdl_host *host, bool captured);
bool sdl_host_present(sdl_host *host, bool pixels_changed);
bool sdl_host_save_screenshot(sdl_host *host, const char *path);

#endif
