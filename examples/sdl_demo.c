#include "demo_app.h"
#include "demo_smoke.h"
#include "sdl_host.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bool smoke;
    int frames;
    const char *screenshot;
} options;

static bool check_wm(wm_status status, const char *operation)
{
    if (status != WM_OK) {
        fprintf(stderr, "%s: %s\n", operation, wm_status_string(status));
        return false;
    }
    return true;
}

static bool parse_options(int argc, char **argv, options *result)
{
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke-test") == 0) {
            result->smoke = true;
        } else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            result->screenshot = argv[++i];
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            char *end;
            errno = 0;
            long count = strtol(argv[++i], &end, 10);
            if (errno != 0 || *end != '\0' || count < 1 || count > INT_MAX) {
                fprintf(stderr, "--frames requires a positive integer\n");
                return false;
            }
            result->frames = (int)count;
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }
    if (result->smoke && result->frames != 0) {
        fprintf(stderr, "--smoke-test and --frames are mutually exclusive\n");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        puts("retro_wm_demo [--frames N | --smoke-test] [--screenshot output.bmp]");
        return EXIT_SUCCESS;
    }
    options opts = {0};
    if (!parse_options(argc, argv, &opts)) return EXIT_FAILURE;

    sdl_host host;
    const sdl_host_config config = {
        .title = "Retro WM | F10:menus  N:new  C:theme  Tab:focus  Ctrl+W:close  Q:quit",
        .width = DEMO_WIDTH, .height = DEMO_HEIGHT,
        .frame_delay_ms = opts.smoke ? 0 : 16, .resizable = true
    };
    if (!sdl_host_init(&host, &config)) return EXIT_FAILURE;

    ui_context ui;
    demo_app app;
    demo_smoke smoke = {0};
    int exit_code = EXIT_FAILURE;
    if (!check_wm(demo_app_init(&app, &ui, &host.framebuffer), "initialize application")) {
        goto cleanup;
    }
    uint64_t frame = 0;
    while (app.running && (opts.frames == 0 || frame < (uint64_t)opts.frames) &&
           (!opts.smoke || frame < DEMO_SMOKE_FRAMES)) {
        if (opts.smoke && !demo_smoke_enqueue(&smoke, &host, &ui, frame)) goto cleanup;
        sdl_host_event event;
        while (sdl_host_poll_event(&host, &event)) {
            switch (event.type) {
            case SDL_HOST_QUIT: app.running = false; break;
            case SDL_HOST_REPAINT: ui_invalidate(&ui); break;
            case SDL_HOST_INPUT:
                if (!check_wm(demo_app_handle_event(&app, &event.input),
                              "handle application input")) {
                    goto cleanup;
                }
                break;
            }
            sdl_host_request_capture(&host, ui_has_capture(&ui));
        }
        bool repaint = ui_needs_repaint(&ui);
        if (repaint && !check_wm(ui_render(&ui), "render interface")) goto cleanup;
        if (!sdl_host_present(&host, repaint)) goto cleanup;
        ++frame;
    }
    if (opts.smoke && !demo_smoke_verify(&smoke, &ui)) goto cleanup;
    if (opts.screenshot != NULL && !sdl_host_save_screenshot(&host, opts.screenshot)) {
        goto cleanup;
    }
    exit_code = EXIT_SUCCESS;

cleanup:
    sdl_host_destroy(&host);
    return exit_code;
}
