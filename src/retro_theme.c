#include "retro_theme.h"

wm_theme wm_theme_monochrome(void)
{
    const wm_color black = WM_RGB(0, 0, 0);
    const wm_color white = WM_RGB(255, 255, 255);
    return (wm_theme){
        .desktop = {
            .background = white, .alternate_background = black
        },
        .window = {
            .frame = black, .client_background = white,
            .active_title_background = black, .inactive_title_background = white,
            .active_title_text = white, .inactive_title_text = black
        },
        .menu = {
            .bar_background = white, .bar_text = black,
            .active_bar_background = black, .active_bar_text = white,
            .dropdown_background = white, .dropdown_text = black,
            .selected_item_background = black, .selected_item_text = white,
            .border = black, .separator = black, .shadow = black
        },
        .outline = {
            .foreground = black, .alternate = white
        }
    };
}

wm_theme wm_theme_colour(void)
{
    const wm_color dark = WM_RGB(25, 33, 48);
    const wm_color light = WM_RGB(245, 241, 224);
    const wm_color blue = WM_RGB(42, 100, 132);
    const wm_color white = WM_RGB(255, 255, 255);
    return (wm_theme){
        .desktop = {
            .background = WM_RGB(64, 64, 208),
            .alternate_background = WM_RGB(54, 100, 117)
        },
        .window = {
            .frame = dark, .client_background = light,
            .active_title_background = blue,
            .inactive_title_background = WM_RGB(177, 190, 207),
            .active_title_text = white, .inactive_title_text = dark
        },
        .menu = {
            .bar_background = light, .bar_text = dark,
            .active_bar_background = blue, .active_bar_text = white,
            .dropdown_background = light, .dropdown_text = dark,
            .selected_item_background = blue, .selected_item_text = white,
            .border = dark, .separator = dark, .shadow = dark
        },
        .outline = {
            .foreground = WM_RGB(255, 215, 72), .alternate = dark
        }
    };
}
