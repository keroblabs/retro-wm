#ifndef RETRO_THEME_H
#define RETRO_THEME_H

#include "retro_types.h"

typedef struct {
    wm_color background, alternate_background;
} wm_desktop_colours;

typedef struct {
    wm_color frame, client_background;
    wm_color active_title_background, inactive_title_background;
    wm_color active_title_text, inactive_title_text;
} wm_window_colours;

typedef struct {
    wm_color bar_background, bar_text;
    wm_color active_bar_background, active_bar_text;
    wm_color dropdown_background, dropdown_text;
    wm_color selected_item_background, selected_item_text;
    wm_color border, separator, shadow;
} wm_menu_colours;

typedef struct {
    wm_color foreground, alternate;
} wm_outline_colours;

typedef struct {
    wm_desktop_colours desktop;
    wm_window_colours window;
    wm_menu_colours menu;
    wm_outline_colours outline;
} wm_theme;

wm_theme wm_theme_monochrome(void);
wm_theme wm_theme_colour(void);

#endif
