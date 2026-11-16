# Bugs — Fonts

Scope: Bitmap font descriptors, encoding, glyph rendering, font selection and conversion tools.  
ID prefix: `RWM-FNT`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `RWM-FNT-0001` | `OPEN` | Support extensible bitmap font descriptors and encodings |
| `RWM-FNT-0002` | `OPEN` | Allow independent fonts for menus, titles and client text |
| `RWM-FNT-0003` | `OPEN` | Generalize and test bitmap font generation |

## Issues

### RWM-FNT-0001

**ID:** `RWM-FNT-0001`  
**Status:** `OPEN`  
**Title:** Support extensible bitmap font descriptors and encodings

**Description**  
The font renderer only accepts the two built-in 4-bit fonts. The public font definition advertises other font types without an encoding-independent rendering contract, limiting reuse and smaller monochrome fonts.

**Details**  
**Confirmed on `main` (2026-11-16):** `include/retro_font.h` defines `font_data_t` with width, height, offset, size, `type`, and data, but no explicit glyph count or pixel-encoding field. `src/retro_wm.c::wm_canvas_text_font` asserts that the pointer equals `font_c8x14` or `font_c12x20`; `draw_character` always reads packed 4-bit levels and assumes the final character is `0x7f`.

**Acceptance criteria for review:** An external, valid font descriptor can be rendered without adding its address to a whitelist; glyph bounds and fallback are validated; at least monochrome and existing 4-bit greyscale formats are supported; existing fonts retain their appearance; invalid descriptors cannot access glyph data out of bounds. Document byte/bit order and storage calculations.

**Related:** `RWM-FNT-0003`. This ticket defines the contract, not the production font assets.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Review a small descriptor extension specifying glyph range/count, data size and a bitmap encoding independent of font dimensions or font identity. Dispatch rendering by encoding; keep a straightforward embedded implementation and preserve compatibility where practical.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-FNT-0002

**ID:** `RWM-FNT-0002`  
**Status:** `OPEN`  
**Title:** Allow independent fonts for menus, titles and client text

**Description**  
Menus and window chrome always use the 8×14 font and hardcoded text dimensions, preventing independent font selection and correct re-layout when fonts change.

**Details**  
**Confirmed:** `include/retro_wm.h` defines `WM_TEXT_WIDTH`, `WM_TEXT_HEIGHT` and `WM_TITLE_HEIGHT` as constants; `src/retro_menu.c::rm_init` uses those values to size menus; `src/retro_wm.c::draw_window` and `wm_canvas_text` use the built-in 8×14 font. Client callbacks may explicitly use 12×20.

**Acceptance criteria for review:** Menu and title font choices are independent; text measurement and related geometry derive from selected font metrics; clipping, interaction hit targets and minimum sizes remain consistent; legacy defaults look unchanged; tests cover at least two font sizes.

**Dependencies:** `RWM-FNT-0001`; coordinate theme ownership with `RWM-CFG-0002` and runtime updates with `RWM-RUN-0001`.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Introduce a toolkit-level font selection contract, with text measurement/line spacing helpers as appropriate. Prefer configuration through the theme/style interface rather than hardcoded globals; do not require every client to use the chrome font.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-FNT-0003

**ID:** `RWM-FNT-0003`  
**Status:** `OPEN`  
**Title:** Generalize and test bitmap font generation

**Description**  
Font generation is geared to fixed grid images and 4-bit greyscale output, so new fonts and formats need a documented, reproducible asset workflow.

**Details**  
**Confirmed:** `tools/convert_font.py` extracts a configurable grid into nibble-packed greyscale C arrays with a default 96-glyph ASCII range; the generated sources are compiled directly. This does not yet cover a monochrome encoding or validate generated assets against a general font contract.

**Acceptance criteria for review:** Generation is reproducible from source assets, emits metadata compatible with `RWM-FNT-0001`, validates input bounds and glyph count, and has test fixtures for at least the existing 4-bit format plus a monochrome example. Preserve existing visual output unless a separately reviewed change requires it.

**Dependency:** `RWM-FNT-0001`.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Extend the existing converter and accompanying tests rather than replacing them without reason. Decide generation options and whether font assets are compiled into the binary before implementation.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
