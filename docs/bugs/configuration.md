# Bugs — Configuration

Scope: Compile-time build options, resource limits and UI theme/style definitions.  
ID prefix: `RWM-CFG`  
Methodology: [bug-tracking-methodology.md](bug-tracking-methodology.md)

The summary is the current index; full issue records below are authoritative.
Update the summary whenever a ticket title or status changes. Keep dated,
append-only Notes, and record each implementation commit and its review status.
Leave Resolution Description as `Pending` until an outcome has been reviewed.
Follow the status rules in the methodology.

## Summary

| ID | Status | Title |
| --- | --- | --- |
| `RWM-CFG-0001` | `OPEN` | Select logical display resolution at compile time |
| `RWM-CFG-0002` | `OPEN` | Expand themes beyond colours to fonts and visual metrics |
| `RWM-CFG-0003` | `OPEN` | Allow build-time configuration of toolkit capacity limits |
| `RWM-CFG-0004` | `OPEN` | Document a repeatable SDL-free dual-pixel-format regression |

## Issues

### RWM-CFG-0001

**ID:** `RWM-CFG-0001`  
**Status:** `OPEN`  
**Title:** Select logical display resolution at compile time

**Description**  
The demonstration assumes a 960×640 logical desktop. A smaller target such as 640×480 cannot use its existing fixed initial window positions, preventing straightforward cross-builds.

**Details**  
**Confirmed:** `examples/demo_app.h` sets `DEMO_WIDTH = 960`, `DEMO_HEIGHT = 640`; `examples/demo_app.c` defines fixed initial window bounds, including `{570,80,330,218}` (ending at X=900); `examples/sdl_demo.c` passes these dimensions to the host. SDL host window resizing scales its logical display but does not change the UI's logical dimensions.

**Decision (2026-11-16):** This is a **compile-time configuration** task, not a runtime resolution-switching feature (revision of the earlier informal `RWM-03` roadmap item).

**Acceptance criteria for review:** Select target logical width/height at build time with a documented default; initial demo windows and menus fit 640×480 and 960×640 without invalid window creation; test both sizes under RGB565 and RGB888 where applicable; ensure all build targets agree on the selected values. Runtime display resizing/reflow is **out of scope**.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.
- **2026-11-16 — Decision:** Logical display resolution is selected at compile time; live resolution switching is excluded from this task. The previous informal roadmap label `RWM-03` is superseded by the permanent ticket `RWM-CFG-0001`.


**Proposed Change**  
Provide explicit CMake/cache or build defines for the logical desktop size and a portable default configuration header for non-CMake builds. Derive demo placement from configured dimensions or supply deliberate preset layouts. Preserve SDL host scaling of a chosen logical framebuffer without requiring live resize.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-CFG-0002

**ID:** `RWM-CFG-0002`  
**Status:** `OPEN`  
**Title:** Expand themes beyond colours to fonts and visual metrics

**Description**  
`wm_theme` currently describes colours only. Font selection and UI dimensions are distributed through compile-time constants, limiting cohesive visual styles.

**Details**  
**Confirmed:** `include/retro_theme.h` groups `desktop`, `window`, `menu`, and `outline` colours; `src/retro_theme.c` defines colour/monochrome presets. `WM_BORDER`, title/menu dimensions and text dimensions are not theme-owned.

**Acceptance criteria for review:** Define which font and style metrics are theme-owned, ensure renderer and hit testing share them, retain existing colour and monochrome presets, and document validation and compatibility. Do not combine this ticket with a full skin/resource manager.

**Related:** `RWM-FNT-0002` handles the font selection mechanics; `RWM-RUN-0001` handles safely applying changed metrics at runtime.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Review a style structure with separate colour, typography and geometry sections. Preserve the semantic-colour structure and keep window behaviour separate from appearance. Decide whether all metrics can change at runtime or some must remain build-time constants.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-CFG-0003

**ID:** `RWM-CFG-0003`  
**Status:** `OPEN`  
**Title:** Allow build-time configuration of toolkit capacity limits

**Description**  
Fixed limits on windows, menus and menu items may not fit all future resource-constrained or larger targets.

**Details**  
**Confirmed:** `WM_MAX_WINDOWS = 8` in `include/retro_wm.h`; `RM_MAX_MENUS = 8` and `RM_MAX_ITEMS = 16` in `include/retro_menu.h`. These size fixed arrays in caller-owned context structures.

**Acceptance criteria for review:** Limits remain deterministic at compile time, are consistently applied across modules and tests, and fail clearly on invalid configurations. Avoid dynamic allocation in the core solely to satisfy this ticket.

**Related:** `RWM-CFG-0001` for build configuration conventions.

**Notes**
- **2026-11-16 — Discovery:** Registered from the Retro WM architectural review and follow-up task planning. Initial evidence is a source inspection; implementation and target hardware validation have not been carried out.


**Proposed Change**  
Consider a central compile-time configuration header or CMake definitions with documented defaults and minimum/maximum constraints. Verify object size and API compatibility before adopting a mechanism.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.


### RWM-CFG-0004

**ID:** `RWM-CFG-0004`  
**Status:** `OPEN`  
**Title:** Document a repeatable SDL-free dual-pixel-format regression

**Description**  
The README documents individual builds, but a small standalone validation guide would make repeatable SDL-free testing of both supported pixel formats easier for developers and automated workers.

**Details**  
Source baseline: main commit `9cad558229736da7a7316ed24809474980226aeb`. Designated working branch: `factory-m1-trial`. Acceptance: create only `docs/core-regression.md` with copyable CMake configure/build/CTest commands for RGB888 and RGB565, separate build directories, explicit SDL-free and testing settings, expected failure handling, and an explanation of what the tests do not verify. Use existing README and CMakeLists.txt as evidence. No source, build-system or existing documentation changes. The operator may independently run both build/test sequences; do not claim results not observed.

**Notes**
- **2026-11-24 — Discovery:** Selected as a bounded first Software Factory integration fixture. This ticket records the proposed task; execution requires explicit operator approval of the pinned revision. Human review remains mandatory.

**Proposed Change**  
Add the standalone Markdown validation guide. Keep the current project build and existing ticket methodology unchanged.

**Commits**

| Branch | Commit (short hash) | Review Status | Description |
| --- | --- | --- | --- |

**Resolution Description**  
Pending.
