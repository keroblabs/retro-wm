# Retro WM

A small, standalone C window-manager prototype exploring interaction behavior
similar to Apple II MGTK. This is a new implementation, not a translation of the
assembly, and does not include Apple II fonts, icons, binaries, or resources.

The initial scope is overlapping windows, activation, captured pointer input,
outline dragging/resizing, close-button tracking, keyboard window switching,
repainting exposed content, and a desktop-style drop-down menu bar. Widgets,
submenus, scrolling, text entry, and exact MGTK compatibility are not implemented.

## Build and run

Requires a C11 compiler, CMake 3.16+, pkg-config, and SDL2 development files.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/retro_wm_demo
```

### Optional generated floppy resource

The default core and SDL demo do not require generated icon assets, artwork,
Python, or Pillow. To compile and link only the generated floppy resource into
`retro_wm_demo`, generate it and enable the opt-in build option:

```sh
python3 tools/convert_icons_rgb565.py --crop-config icons/images/crop-proposals.json
cmake -S . -B build-icon-demo -DRETRO_WM_BUILD_DEMO=ON -DRETRO_WM_BUILD_ICON_DEMO=ON
cmake --build build-icon-demo
```

`RETRO_WM_BUILD_ICON_DEMO` defaults to `OFF` and requires
`RETRO_WM_BUILD_DEMO=ON`. If either generated floppy C resource file is missing,
CMake reports the missing path and the generation command. After editing manual
pixel maps, regenerate the assets with `--cleanup` before building; see the
[icon preparation guide](icons/images/README.md#editable-pixel-maps-and-manual-cleanup).
This only adds the resource to the executable; it does not register or render
an icon at runtime. Generated files are ignored by Git.

The demo opens at 960x640 logical pixels. The host window can be resized; SDL
scales/letterboxes the framebuffer and maps pointer input to logical coordinates.
Colour is enabled by default.

### Compile-time pixel format

Select the framebuffer format when configuring the build:

```sh
cmake -S . -B build-rgb565 -DRETRO_WM_PIXEL_FORMAT=RGB565 -DCMAKE_BUILD_TYPE=Debug
cmake --build build-rgb565
ctest --test-dir build-rgb565 --output-on-failure
./build-rgb565/retro_wm_demo
```

`RETRO_WM_PIXEL_FORMAT=RGB888` is the default and preserves the existing layout:

| Selection | `wm_pixel` storage | Native layout |
|---|---|---|
| `RGB565` | `uint16_t`, 2 bytes | `rrrrrggggggbbbbb` |
| `RGB888` | `uint32_t`, 4 bytes | `0xAARRGGBB`, normally opaque alpha |

RGB888 means eight bits per RGB channel in the existing **32-bit word**, not
packed 24-bit storage. At 960x640 with no row padding, the framebuffer occupies
1,228,800 bytes in RGB565 or 2,457,600 bytes in RGB888.

The public [retro_pixel.h](include/retro_pixel.h) macro is `WM_PIXEL_FORMAT`,
set to `WM_PIXEL_FORMAT_RGB565` or `WM_PIXEL_FORMAT_RGB888`. CMake exports this
definition through both toolkit and SDL-adapter targets. Without CMake, pass
`-DWM_PIXEL_FORMAT=WM_PIXEL_FORMAT_RGB565` consistently to every translation unit,
or omit it for RGB888. Unsupported formats fail configuration/compilation.
Rebuild all components when changing formats; do not mix libraries compiled
with different pixel settings.

This is not a runtime setting. C/View still switch colour palettes, independently
of pixel storage. Logical `wm_color` values and `WM_RGB` remain eight-bit-channel
colours in either build; the renderer packs them to RGB565 when writing pixels.

Controls:

| Input | Action |
|---|---|
| Click a menu title | Open its drop-down; click an item to select |
| Press a menu title and drag | Track across menus/items; release to select |
| F10 | Open/close keyboard menu mode |
| Arrow keys / Enter | Navigate menus / choose an enabled item |
| Click outside an open menu | Dismiss without clicking through |
| Click a window | Activate and raise it |
| Drag its title bar | Move an outline; release to commit |
| Drag its lower-right grip | Resize an outline; release to commit |
| Escape | Cancel the current interaction |
| Press/release the close box | Close, if released inside the box |
| Tab / Shift+Tab | Cycle window activation |
| Ctrl+W | Close the active window |
| N | Create another example window |
| C | Toggle monochrome / colour |
| Q | Quit the SDL host |

Focus loss cancels a pending interaction. Alt+F4 is also understood by the core,
but a desktop environment may intercept it and close the entire host instead.
There is an eight-window limit. Windows stay below the menu bar and within the
logical display. While a menu is open it receives input before the windows and
host shortcuts; Escape dismisses it without triggering another action.

The File menu creates/closes windows and quits the host; View selects Colour or
Monochrome; Window cycles activation; Help opens/activates the controls window.
Disabled items are stippled, separators divide groups, and a checkmark indicates
the current colour mode. The C shortcut also toggles themes when menus are closed.
Changing themes does not change window logic or input handling.

## Boundaries

```text
platform/sdl/sdl_host       Future RISC-V adapter
       |                          |
       +---- wm_event / framebuffer ----+
                         |
              application event loop
              /                    \
      demo_app (replaceable)       ui_context (reusable)
      content and commands        /                \
                            wm_context         rm_context
```

There are three distinct state types. The SDL host owns platform resources;
`demo_app` owns only example application state and borrows a `ui_context`; the
UI context owns the menu/window composition. No SDL API accepts demo/application
state, and no toolkit API knows the demo's menu command IDs.

* [include/retro_types.h](include/retro_types.h): shared input, geometry, colour,
  framebuffer and status types; no platform or window-manager state.
* [include/retro_pixel.h](include/retro_pixel.h): compile-time pixel selection,
  native storage type and logical-colour packing.
* [include/retro_theme.h](include/retro_theme.h) and
  [src/retro_theme.c](src/retro_theme.c): semantic colour roles and the established
  colour/monochrome presets, separate from renderer implementations.
* [include/retro_wm.h](include/retro_wm.h): public C API and statically allocated
  window-manager context.
* [src/retro_wm.c](src/retro_wm.c): window state, nonblocking interactions,
  software painting, and packed 4-bit ASCII font rendering.
* [include/retro_font.h](include/retro_font.h),
  [src/retro_font_c8x14.c](src/retro_font_c8x14.c), and
  [src/retro_font_c12x20.c](src/retro_font_c12x20.c): font metadata and bitmap data.
* [include/retro_menu.h](include/retro_menu.h) and
  [src/retro_menu.c](src/retro_menu.c): independent, descriptor-driven menu state,
  input handling, layout, and painting using the shared drawing/event API, with
  no SDL calls.
* [include/retro_ui.h](include/retro_ui.h) and
  [src/retro_ui.c](src/retro_ui.c): reusable menu/window composition, event
  priority, pointer-coordinate conversion, capture and repainting.
* [platform/sdl/sdl_host.h](platform/sdl/sdl_host.h) and
  [platform/sdl/sdl_host.c](platform/sdl/sdl_host.c): SDL resource ownership,
  normalized input, native pointer capture, framebuffer allocation/presentation,
  frame pacing and screenshot output. Only shared types cross this boundary.
* [examples/demo_app.h](examples/demo_app.h) and
  [examples/demo_app.c](examples/demo_app.c): SDL-free example panels, menu
  descriptors, application-owned content colours, commands and shortcuts.
* [examples/sdl_demo.c](examples/sdl_demo.c): thin entry point wiring the three
  state objects together; it contains no SDL calls or widget implementation.
* [examples/demo_smoke.c](examples/demo_smoke.c): demo-specific SDL event injection
  and assertions, separate from application and platform code.
* [tests/test_wm.c](tests/test_wm.c): SDL-free window, menu and composition tests.

The core has no SDL, operating-system, clock, interrupt, heap-allocation, or
CPU-specific calls. It uses standard C memory/string routines. Window records
and interaction state live in a caller-owned `wm_context`.

### Starting a new application

Use [examples/sdl_demo.c](examples/sdl_demo.c) as the platform entry-point pattern,
and replace `demo_app.c/.h` with your own application. Keep `retro_wm`,
`retro_menu`, `retro_ui` and the SDL host unchanged:

1. Initialize an `sdl_host` using application-supplied dimensions and title.
2. Keep a separate `ui_context` and application state object. Initialize the UI
   with the host framebuffer and your application's menu descriptors.
3. Use `wm_` functions on `ui.windows` and `rm_` functions on `ui.menus`. These
   are explicitly exposed component handles, not duplicated wrapper APIs.
4. Dispatch normalized input through `ui_handle_event`. It returns opaque menu
   command IDs, menu-consumption state and window-change notifications. Execute
   application commands yourself; do not interpret shortcuts when
   `menu_consumed` is true.
5. Translate host quit/repaint notifications in the entry point. Synchronize
   native capture using `ui_has_capture`, render when `ui_needs_repaint`, and
   present the framebuffer through `sdl_host_present`.

`ui_render` redraws windows and menus with the window manager's current theme.
Applications call `wm_set_theme(&ui.windows, ...)` rather than synchronizing two
independent palettes. The UI never executes commands, decides when to quit,
allocates display memory, or calls SDL.

The host owns and frees framebuffer pixels. The UI borrows that buffer;
applications own menu descriptors and paint-callback user data. Keep all three
objects alive at stable addresses until the event loop stops, then destroy the
host. Do not copy initialized contexts or application instances containing
borrowed pointers. Frame counters and smoke-test identities are not application
or toolkit state.

The CMake targets follow the same boundaries: `retro_wm` is the portable toolkit,
`retro_sdl_host` is the platform adapter, and `retro_demo_app` is the replaceable,
SDL-free application. The executable only links and wires these components.

### Theme colours

`wm_theme` is defined in [retro_theme.h](include/retro_theme.h), with separate
groups so unrelated UI elements no longer share a colour setting:

| Group | Roles |
|---|---|
| `desktop` | Background and alternate pattern colour |
| `window` | Frame, client background, active/inactive title backgrounds and title text |
| `menu` | Bar background/text, active bar background/text, drop-down background/text, selected item background/text, border, separator and shadow |
| `outline` | Foreground and alternate colour for drag/resize outlines |

Menu-bar and drop-down backgrounds are **independent fields**. Both initially
use the existing cream colour; menu selections and active window titles likewise
retain their established blue values without sharing mutable settings.

Start with `wm_theme_colour()` or `wm_theme_monochrome()`, edit individual roles,
and apply the complete struct through `wm_set_theme`. For example, this keeps the
current drop-down colour while showing the field to customize:

```c
wm_theme theme = wm_theme_colour();
theme.menu.dropdown_background = WM_RGB(245, 241, 224);
wm_status status = wm_set_theme(&ui.windows, &theme);
if (status != WM_OK) {
    fprintf(stderr, "Theme: %s\n", wm_status_string(status));
}
```

Applying a theme schedules repainting of both windows and menus through the UI
composition. Fields are now accessed through `theme.desktop`, `theme.window`,
`theme.menu`, and `theme.outline`, replacing the old flat field names.

Application content is intentionally separate: `demo_colours` owns the demo's
panel backgrounds, text and sample-bar fills. Paint callbacks read that struct,
not hardcoded colours. The demo's View/C controls switch both the toolkit preset
and its application palette. No demo-specific colours are built into the toolkit.
After editing application palette values directly, call `ui_invalidate` to
repaint client content, just as for other application-state changes.

### Input and lifecycle

1. Initialize a context with `wm_init`.
2. Create windows with `wm_create_window`; keep each callback's user data alive
   until that window closes. Titles are copied into the context.
3. Translate platform events and call `wm_handle_event`. Calls never run their
   own polling/modal loops.
4. Inspect the optional `wm_update` for activation, movement, resize, or close
   notifications.
5. When `wm_needs_repaint` is true, call `wm_render`, then present the framebuffer.
   Call `wm_invalidate` when client content changes.

Only primary-button chrome interactions are handled. The current keyboard
contract covers window switching, cancellation and closing; application-level
client input/text entry is not implemented. Modifier bits are generic
Shift/Ctrl/Alt/Meta, not Apple-specific.

Use the API to access context state; its concrete storage is exposed only so
embedded callers can allocate it statically. Contexts are independent, but each
context must be used serially. Do not mutate it from paint callbacks or interrupt
handlers. A device driver should queue input for the application's main loop.

Operations returning `wm_status` report invalid arguments, missing windows and
capacity errors explicitly. Query/drawing functions require valid initialized
objects; debug builds assert those preconditions.

### Framebuffer and painting

The caller supplies at least `stride_pixels * height` elements of `wm_pixel`.
Use `sizeof(wm_pixel)` for allocation and byte-pitch calculations; it is two
bytes in RGB565 builds or four in RGB888 builds. Pixel words use native byte
order. Stride is in **pixels**, can exceed width, and padding is not modified.
Drawing APIs accept logical `wm_color` values and perform storage conversion,
so clients and themes do not need format-specific colour definitions.
Use `wm_color_to_pixel` when writing framebuffer pixels directly; do not cast
an array of logical colours to a native pixel array.

Client callbacks receive a content-local canvas clipped to their content area,
and repaint synchronously. Clients own their application state, not persistent
pixel surfaces. The core redraws the scene bottom-to-top when requested, then
draws any interaction outline. This intentionally avoids XOR drawing and
save-behind buffers: closing/cancelling cannot leave old outlines behind.

`wm_render` always paints a complete frame. Dirty tracking is a repaint hint,
not a damaged-region implementation. This favors a simple, testable prototype;
partial redraw can be added later if measurements require it.

`wm_canvas_text` uses the 8x14 font, including lowercase ASCII. Menus and title
bars always use this font; their widths and vertical padding follow
`WM_TEXT_WIDTH` and `WM_TEXT_HEIGHT`. Use `wm_canvas_text_font` with
`&font_c8x14` or `&font_c12x20` to select a font for client content. The demo
uses 12x20 client text. Characters advance by the selected font width, and
newlines advance by its height plus four pixels. Bytes outside `0x20`–`0x7f`
use a question mark; this is not a Unicode text renderer.

Each 8x14 glyph occupies 56 bytes: four bytes per row, top to bottom, with the
left pixel in bits `[3:0]` and the next pixel in bits `[7:4]`. The 12x20 font
uses the same packing with six bytes per row and 120 bytes per glyph. Before
drawing text, paint its background. Each level is linear opacity: the renderer
blends the requested text colour with the existing pixel using
`(text * level + background * (15 - level) + 7) / 15` per eight-bit RGB channel.
Level 0 leaves the existing pixel untouched; level 15 is the exact requested
colour, converted to native storage. RGB565 background channels are expanded
to eight bits before blending, then the result is packed back to RGB565.
This preserves smooth edges for dark text on light backgrounds as well as
light text on dark backgrounds. All theme text roles use this same path.

### Menu API

Create caller-owned `rm_item` arrays and `rm_menu` descriptors, then call `rm_init`
with the full display dimensions. A command item has a nonzero application
command ID, label, optional displayed shortcut, and enabled/checked state.
Separators use `RM_ITEM_SEPARATOR`. Keep descriptors and strings alive for the
context's lifetime; only enabled/checked flags may change without reinitializing.
Call `rm_invalidate` after changing those flags.

Feed full-display `wm_event` coordinates to `rm_handle_event` before dispatching
to windows, except that an already-captured window interaction must retain its
pointer events even over the menu bar. Its `rm_result` indicates whether input
was consumed and whether a command was selected. Execute commands in the application, not in the menu
module. Displayed shortcut strings are labels; the application implements the
bindings. F10, arrows, Enter/Space, Escape, and pointer tracking are built in.

When either layer needs repainting, repaint the desktop/windows first, then call
`rm_render` over the full framebuffer with the same theme. Menu dismissal does
not restore saved pixels itself. This composition avoids stale pop-up shadows
or outlines and keeps both modules nonblocking and allocation-free.

`ui_context` implements that routing contract, reserves `RM_BAR_HEIGHT` rows
using a framebuffer view, and translates pointer Y coordinates at that boundary.
Applications using `ui_` APIs do not need to reproduce this glue. Window logic
therefore remains independent of menu layout. Opening a menu cancels an active
window interaction; focus loss cancels both.

The current limits are eight menus and sixteen items per menu. Menu titles must
fit across the bar and drop-down heights must fit below it; `rm_init` reports
unsupported layouts rather than hiding unreachable items. Wide drop-downs are
kept on-screen and their text is clipped.

## Validation without a desktop session

```sh
SDL_VIDEODRIVER=dummy ./build/retro_wm_demo --smoke-test
SDL_VIDEODRIVER=dummy ./build/retro_wm_demo --frames 1 --screenshot colour.bmp
SDL_VIDEODRIVER=dummy ./build/retro_wm_demo --smoke-test --screenshot menu.bmp
```

The smoke test injects SDL events through the same translation path as real
input, and checks drag completion, focus cycling, close handling, mouse menu
selection, keyboard menu navigation, theme commands, and outside-click dismissal
without window activation. It also checks a captured drag released over the menu
bar. It ends with a highlighted drop-down for screenshot capture. It also uploads
and presents the software framebuffer. It does not
replace testing native desktop pointer capture or physical display hardware.

For SDL-free development on the host (including compilation of the portable demo
application when tests are enabled):

```sh
cmake -S . -B build-core -DRETRO_WM_BUILD_DEMO=OFF
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

For host-side memory/undefined-behavior checks:

```sh
cmake -S . -B build-sanitize -DRETRO_WM_BUILD_DEMO=OFF \
  -DRETRO_WM_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-sanitize
ctest --test-dir build-sanitize --output-on-failure
```

For the RGB565 sanitizer variant, add `-DRETRO_WM_PIXEL_FORMAT=RGB565` and use a
separate build directory such as `build-rgb565-sanitize`.

## Later RISC-V integration

Keep the toolkit/composition and application APIs; replace only
`platform/sdl/sdl_host` and the entry-point wiring with a board-specific adapter:

* Supply framebuffer storage and feed normalized input events.
* Supply a hardware cursor or software pointer overlay; the SDL demo uses the
  host's native cursor.
* Select RGB565/RGB888 to match the framebuffer. Convert/copy only if the
  hardware's format or byte order differs from the selected native pixel words.
* Handle display pitch, cache maintenance, synchronization and presentation in
  the adapter.
* Provide the usual C runtime memory/string functions and a suitable toolchain.
* Disable host test executables when cross-building (`-DBUILD_TESTING=OFF`).

No RISC-V board support, cross-toolchain configuration, or hardware validation
is included yet. Those depend on the selected board/display/controller.
