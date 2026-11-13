#ifndef DEMO_SMOKE_H
#define DEMO_SMOKE_H

#include "retro_ui.h"
#include "sdl_host.h"

enum { DEMO_SMOKE_FRAMES = 24 };

typedef struct {
    wm_id initial_ids[3], created;
    wm_rect initial_bounds[3];
} demo_smoke;

bool demo_smoke_enqueue(demo_smoke *smoke, const sdl_host *host,
                         const ui_context *ui, uint64_t frame);
bool demo_smoke_verify(const demo_smoke *smoke, const ui_context *ui);

#endif
